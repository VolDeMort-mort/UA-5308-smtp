#include "MailWorker.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include "AttachmentHandler.hpp"
#include "Base64Decoder.hpp"
#include "Base64Encoder.hpp"
#include "ClientSecureChannel.hpp"
#include "Email.h"
#include "ILogger.h"
#include "MimeBuilder.h"
#include "MimeParser.h"
#include "SocketConnection.hpp"
#include "SocketConnector.hpp"

namespace
{
bool WaitForReconnectDelay(std::atomic<bool>& stopRequested, std::chrono::milliseconds delay)
{
    constexpr auto kSlice = std::chrono::milliseconds(100);
    auto waited = std::chrono::milliseconds(0);

    while (waited < delay)
    {
        if (stopRequested.load(std::memory_order_relaxed))
        {
            return false;
        }

        const auto step = std::min(kSlice, delay - waited);
        std::this_thread::sleep_for(step);
        waited += step;
    }

    return !stopRequested.load(std::memory_order_relaxed);
}

std::string QuoteImap(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size() + 2);
    escaped.push_back('"');
    for (char c : value)
    {
        if (c == '\\' || c == '"')
        {
            escaped.push_back('\\');
        }
        escaped.push_back(c);
    }
    escaped.push_back('"');
    return escaped;
}

std::string ToLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string TrimCopy(const std::string& value)
{
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
    {
        return {};
    }

    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

std::vector<std::string> SplitAddressList(const std::string& raw)
{
    std::vector<std::string> out;
    std::string current;
    bool inQuotes = false;
    int angleDepth = 0;

    for (const char c : raw)
    {
        if (c == '"' && angleDepth == 0)
        {
            inQuotes = !inQuotes;
            current.push_back(c);
            continue;
        }

        if (!inQuotes && c == '<')
        {
            ++angleDepth;
            current.push_back(c);
            continue;
        }

        if (!inQuotes && c == '>')
        {
            if (angleDepth > 0)
            {
                --angleDepth;
            }
            current.push_back(c);
            continue;
        }

        if (!inQuotes && angleDepth == 0 && c == ',')
        {
            const std::string token = TrimCopy(current);
            if (!token.empty())
            {
                out.push_back(token);
            }
            current.clear();
            continue;
        }

        current.push_back(c);
    }

    const std::string token = TrimCopy(current);
    if (!token.empty())
    {
        out.push_back(token);
    }

    return out;
}

std::string CollapseWhitespace(const std::string& value)
{
    std::string out;
    out.reserve(value.size());

    bool prevSpace = false;
    for (const char c : value)
    {
        const bool isSpace = std::isspace(static_cast<unsigned char>(c)) != 0;
        if (isSpace)
        {
            if (!prevSpace)
            {
                out.push_back(' ');
                prevSpace = true;
            }
            continue;
        }

        out.push_back(c);
        prevSpace = false;
    }

    return TrimCopy(out);
}

std::string StripHtmlTags(const std::string& html)
{
    std::string out;
    out.reserve(html.size());

    bool inTag = false;
    for (const char c : html)
    {
        if (c == '<')
        {
            inTag = true;
            continue;
        }

        if (c == '>')
        {
            inTag = false;
            continue;
        }

        if (!inTag)
        {
            out.push_back(c);
        }
    }

    return out;
}

std::string BuildPreview(const std::string& plainBody, const std::string& htmlBody, std::size_t maxWords)
{
    std::string base = plainBody.empty() ? StripHtmlTags(htmlBody) : plainBody;
    base = CollapseWhitespace(base);
    if (base.empty() || maxWords == 0)
    {
        return {};
    }

    std::istringstream input(base);
    std::string word;
    std::string out;
    std::size_t count = 0;
    while (input >> word)
    {
        if (count != 0)
        {
            out.push_back(' ');
        }

        out += word;
        ++count;
        if (count >= maxWords)
        {
            break;
        }
    }

    return out;
}

void ExtractFlags(const std::vector<std::string>& lines, bool& isSeen, bool& isFlagged)
{
    isSeen = false;
    isFlagged = false;

    for (const auto& line : lines)
    {
        if (line.find("FLAGS (") == std::string::npos)
        {
            continue;
        }

        isSeen = line.find("\\Seen") != std::string::npos;
        isFlagged = line.find("\\Flagged") != std::string::npos;
        return;
    }
}

std::string NormalizeFlag(const std::string& raw)
{
    if (raw.empty())
    {
        return raw;
    }

    if (raw[0] == '\\' || raw[0] == '$')
    {
        return raw;
    }

    const std::string lower = ToLower(raw);
    if (lower == "seen") return "\\Seen";
    if (lower == "flagged" || lower == "important") return "\\Flagged";
    if (lower == "deleted") return "\\Deleted";
    return raw;
}

bool IsStarredFolder(const std::string& folderName)
{
    return ToLower(folderName) == "starred";
}

bool HasSmtpCode(const std::string& response, int code)
{
    const std::string codeText = std::to_string(code);
    return response.size() >= 3 && response.compare(0, 3, codeText) == 0;
}

bool TryParseSmtpCode(const std::string& response, int& code)
{
    if (response.size() < 3)
    {
        return false;
    }

    if (!std::isdigit(static_cast<unsigned char>(response[0])) ||
        !std::isdigit(static_cast<unsigned char>(response[1])) ||
        !std::isdigit(static_cast<unsigned char>(response[2])))
    {
        return false;
    }

    code = (response[0] - '0') * 100 + (response[1] - '0') * 10 + (response[2] - '0');
    return true;
}

template <typename ReceiveFn>
bool ReadSmtpResponse(ReceiveFn&& receive, std::vector<std::string>& lines)
{
    lines.clear();

    auto appendLogicalLines = [](const std::string& chunk, std::vector<std::string>& out)
    {
        std::size_t start = 0;
        while (start <= chunk.size())
        {
            const auto end = chunk.find('\n', start);
            std::string line = (end == std::string::npos)
                                   ? chunk.substr(start)
                                   : chunk.substr(start, end - start);

            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }

            if (!line.empty())
            {
                out.push_back(std::move(line));
            }

            if (end == std::string::npos)
            {
                break;
            }

            start = end + 1;
        }
    };

    std::string chunk;
    if (!receive(chunk))
    {
        return false;
    }

    appendLogicalLines(chunk, lines);
    if (lines.empty())
    {
        return false;
    }

    int code = 0;
    if (!TryParseSmtpCode(lines.front(), code))
    {
        return true;
    }

    if (lines.front().size() < 4 || lines.front()[3] != '-')
    {
        return true;
    }

    const std::string finalPrefix = std::to_string(code) + " ";
    for (const auto& responseLine : lines)
    {
        if (responseLine.rfind(finalPrefix, 0) == 0)
        {
            return true;
        }
    }

    while (true)
    {
        std::string continuationChunk;
        if (!receive(continuationChunk))
        {
            return false;
        }

        const std::size_t previousSize = lines.size();
        appendLogicalLines(continuationChunk, lines);

        for (std::size_t i = previousSize; i < lines.size(); ++i)
        {
            if (lines[i].rfind(finalPrefix, 0) == 0)
            {
                return true;
            }
        }
    }
}

int GetSmtpCode(const std::vector<std::string>& responseLines)
{
    if (responseLines.empty())
    {
        return -1;
    }

    int code = -1;
    if (!TryParseSmtpCode(responseLines.back(), code))
    {
        return -1;
    }

    return code;
}

std::string JoinSmtpLines(const std::vector<std::string>& lines)
{
    std::string out;
    for (std::size_t i = 0; i < lines.size(); ++i)
    {
        if (i != 0)
        {
            out += " | ";
        }

        out += lines[i];
    }

    return out;
}

bool BuildSmtpPlainAuthPayload(const std::string& username,
                               const std::string& password,
                               std::string& out)
{
    std::vector<uint8_t> raw;
    raw.reserve(username.size() + password.size() + 2);
    raw.push_back(0);
    raw.insert(raw.end(), username.begin(), username.end());
    raw.push_back(0);
    raw.insert(raw.end(), password.begin(), password.end());

    try
    {
        out = Base64Encoder::EncodeBase64(raw);
        out.erase(std::remove(out.begin(), out.end(), '\r'), out.end());
        out.erase(std::remove(out.begin(), out.end(), '\n'), out.end());
        return !out.empty();
    }
    catch (...)
    {
        return false;
    }
}

class SilentLogger final : public ILogger
{
public:
    void Log(LogLevel, const std::string&) override
    {
    }
    void set_strategy(std::shared_ptr<ILoggerStrategy> strategy) override
    {
    }
    void set_level(LogLevel level) override
    {

    }
};

std::vector<std::string> CollectRecipients(const SendMailCommand& cmd)
{
    std::vector<std::string> all;
    all.reserve(cmd.to.size() + cmd.cc.size() + cmd.bcc.size() + cmd.recipients.size());

    auto append = [&all](const std::vector<std::string>& src)
    {
        all.insert(all.end(), src.begin(), src.end());
    };

    append(cmd.to);
    append(cmd.cc);
    append(cmd.bcc);
    append(cmd.recipients);

    std::unordered_set<std::string> seen;
    std::vector<std::string> deduped;
    deduped.reserve(all.size());
    for (const auto& rcpt : all)
    {
        if (rcpt.empty())
        {
            continue;
        }

        if (seen.insert(rcpt).second)
        {
            deduped.push_back(rcpt);
        }
    }

    return deduped;
}

std::string ExtractRfc822Payload(const std::vector<std::string>& lines)
{
    auto tryParseLiteralSize = [](const std::string& line, std::size_t& sizeOut) -> bool
    {
        const auto close = line.rfind('}');
        if (close == std::string::npos || close == 0)
        {
            return false;
        }

        const auto open = line.rfind('{', close);
        if (open == std::string::npos || open + 1 >= close)
        {
            return false;
        }

        const std::string value = line.substr(open + 1, close - open - 1);
        if (value.empty() || !std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isdigit(c) != 0; }))
        {
            return false;
        }

        try
        {
            sizeOut = static_cast<std::size_t>(std::stoull(value));
            return true;
        }
        catch (...)
        {
            return false;
        }
    };

    std::size_t literalSize = 0;
    std::size_t literalStart = std::string::npos;
    for (std::size_t i = 0; i < lines.size(); ++i)
    {
        if (tryParseLiteralSize(lines[i], literalSize))
        {
            literalStart = i + 1;
            break;
        }
    }

    if (literalStart == std::string::npos || literalStart >= lines.size() || literalSize == 0)
    {
        return {};
    }

    std::string payload;
    for (std::size_t i = literalStart; i < lines.size(); ++i)
    {
        payload += lines[i];
        payload += "\n";

        if (payload.size() >= literalSize)
        {
            break;
        }
    }

    if (payload.empty())
    {
        return {};
    }

    if (payload.size() > literalSize)
    {
        payload.resize(literalSize);
    }

    return payload;
}

std::string ExtractEnvelopeSubject(const std::string& line)
{
    const auto envPos = line.find("ENVELOPE (");
    if (envPos == std::string::npos)
    {
        return {};
    }

    auto firstQuote = line.find('"', envPos);
    if (firstQuote == std::string::npos)
    {
        return {};
    }

    auto firstEnd = line.find('"', firstQuote + 1);
    if (firstEnd == std::string::npos)
    {
        return {};
    }

    auto subjectStart = line.find('"', firstEnd + 1);
    if (subjectStart == std::string::npos)
    {
        return {};
    }

    auto subjectEnd = line.find('"', subjectStart + 1);
    if (subjectEnd == std::string::npos)
    {
        return {};
    }

    return line.substr(subjectStart + 1, subjectEnd - subjectStart - 1);
}

std::string SanitizeAttachmentFileName(const std::string& raw)
{
    std::string baseName = std::filesystem::path(raw).filename().string();
    if (baseName.empty())
    {
        return "attachment.bin";
    }

    for (char& c : baseName)
    {
        if (c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
        {
            c = '_';
        }
    }

    return baseName;
}

bool TryDecodeAttachmentBytes(const SmtpClient::Attachment& attachment, std::vector<uint8_t>& out)
{
    out.clear();

    if (attachment.base64_data.empty())
    {
        return false;
    }

    try
    {
        out = Base64Decoder::DecodeBase64(attachment.base64_data);
        return true;
    }
    catch (...)
    {
        out.assign(attachment.base64_data.begin(), attachment.base64_data.end());
        return true;
    }
}
} // namespace

MailWorker::MailWorker(CommandQueue& queue, std::function<void(MailResult)> onResult)
    : m_queue(queue), m_onResult(std::move(onResult))
{
}

MailWorker::~MailWorker()
{
    stop();
}

void MailWorker::start()
{
    m_stopRequested.store(false, std::memory_order_relaxed);
    m_thread = std::thread(&MailWorker::run, this);
}

void MailWorker::stop()
{
    m_stopRequested.store(true, std::memory_order_relaxed);
    m_queue.stop();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

std::string MailWorker::nextTag()
{
    return "A" + std::to_string(++m_tagCounter);
}

std::pair<bool, std::vector<std::string>> MailWorker::imapExchange(const std::string& cmd)
{
	auto imapSend = [&](const std::string& data) -> bool
	{
		if (m_imapSecureConnection)
		{
			return m_imapSecureConnection->Send(data);
		}
		return m_imapConnection->Send(data);
	};

	auto imapRecv = [&](std::string& data) -> bool
	{
		if (m_imapSecureConnection)
		{
			return m_imapSecureConnection->Receive(data);
		}
		return m_imapConnection->Receive(data);
	};

	for (int attempt = 0; attempt < 2; ++attempt)
	{
		if (!m_imapConnection)
		{
			std::string reconnectError;
			if (!reconnectSession(reconnectError))
			{
				return {false, {}};
			}
		}

		const std::string tag = nextTag();
		if (!imapSend(tag + " " + cmd + "\r\n"))
		{
			std::string reconnectError;
			if (attempt == 0 && reconnectSession(reconnectError))
			{
				continue;
			}
			return {false, {}};
		}

		std::vector<std::string> untagged;
		bool transportFailure = false;

		while (true)
		{
			std::string frame;
			if (!imapRecv(frame))
			{
				transportFailure = true;
				break;
			}

			std::vector<std::string> lines;
			std::string remaining = frame;

			while (!remaining.empty())
			{
				auto pos = remaining.find('\n');
				std::string line = (pos == std::string::npos) ? remaining : remaining.substr(0, pos);

				if (!line.empty() && line.back() == '\r')
				{
					line.pop_back();
				}	

				lines.push_back(line);
				remaining = (pos == std::string::npos) ? "" : remaining.substr(pos + 1);
			}

			bool done = false;
			bool ok = false;
			for (const auto& line : lines)
			{
				if (line.rfind(tag + " OK", 0) == 0)
				{
					done = true;
					ok = true;
					break;
				}
				if (line.rfind(tag + " NO", 0) == 0 || line.rfind(tag + " BAD", 0) == 0)
				{
					done = true;
					ok = false;
					break;
				}
				untagged.push_back(line);
			}

			if (done) return {ok, untagged};
		}

		if (!transportFailure)
		{
			return {false, untagged};
		}

		std::string reconnectError;
		if (attempt == 0 && reconnectSession(reconnectError))
		{
			continue;
		}

		return {false, untagged};
	}

	return {false, {}};
}

void MailWorker::run()
{
    while (auto cmd = m_queue.pop())
    {
        std::visit(
            [this](const auto& c)
            {
                using T = std::decay_t<decltype(c)>;

                if constexpr (std::is_same_v<T, ConnectCommand>)
                    handleConnect(c);
                else if constexpr (std::is_same_v<T, DisconnectCommand>)
                    handleDisconnect(c);
                else if constexpr (std::is_same_v<T, SendMailCommand>)
                    handleSendMail(c);
                else if constexpr (std::is_same_v<T, FetchMailsCommand>)
                    handleFetchMails(c);
                else if constexpr (std::is_same_v<T, FetchMailCommand>)
                    handleFetchMail(c);
                else if constexpr (std::is_same_v<T, DeleteMailCommand>)
                    handleDeleteMail(c);
                else if constexpr (std::is_same_v<T, MarkMailReadCommand>)
                    handleMarkMailRead(c);
                else if constexpr (std::is_same_v<T, MoveMailCommand>)
                    handleMoveMail(c);
                else if constexpr (std::is_same_v<T, DownloadAttachmentCommand>)
                    handleDownloadAttachment(c);
                else if constexpr (std::is_same_v<T, FetchFoldersCommand>)
                    handleFetchFolders(c);
                else if constexpr (std::is_same_v<T, CreateFolderCommand>)
                    handleCreateFolder(c);
                else if constexpr (std::is_same_v<T, DeleteFolderCommand>)
                    handleDeleteFolder(c);
                else if constexpr (std::is_same_v<T, AddLabelCommand>)
                    handleAddLabel(c);
                else if constexpr (std::is_same_v<T, RemoveLabelCommand>)
                    handleRemoveLabel(c);
                else if constexpr (std::is_same_v<T, MarkMailStarredCommand>)
                    handleMarkMailStarred(c);
                else if constexpr (std::is_same_v<T, MarkMailImportantCommand>)
                    handleMarkMailImportant(c);
            },
            *cmd);
    }
}

bool MailWorker::authenticateSmtp(SocketConnection& connection,
                                  const std::string& username,
                                  const std::string& password,
                                  std::string& error)
{
    std::vector<std::string> smtpResponse;
    if (!ReadSmtpResponse([&](std::string& line) { return connection.Receive(line); }, smtpResponse) ||
        GetSmtpCode(smtpResponse) != 220)
    {
        error = "SMTP greeting failed";
        return false;
    }

    auto sendPlainSmtp = [&](const std::string& line, std::initializer_list<int> okCodes) -> bool
    {
        if (!connection.Send(line + "\r\n"))
        {
            return false;
        }

        smtpResponse.clear();
        if (!ReadSmtpResponse([&](std::string& responseLine) { return connection.Receive(responseLine); }, smtpResponse))
        {
            return false;
        }

        const int code = GetSmtpCode(smtpResponse);
        for (const int ok : okCodes)
        {
            if (code == ok)
            {
                return true;
            }
        }

        return false;
    };

    if (!sendPlainSmtp("EHLO " + username, {250}))
    {
        error = "SMTP EHLO failed: " + JoinSmtpLines(smtpResponse);
        return false;
    }

    if (!sendPlainSmtp("STARTTLS", {220}))
    {
        error = "SMTP STARTTLS failed: " + JoinSmtpLines(smtpResponse);
        return false;
    }

    ClientSecureChannel secureSmtp(connection);
    if (!secureSmtp.StartTLS())
    {
        error = "SMTP TLS handshake failed";
        return false;
    }

    auto sendSecureSmtp = [&](const std::string& line, std::initializer_list<int> okCodes) -> bool
    {
        if (!secureSmtp.Send(line + "\r\n"))
        {
            return false;
        }

        smtpResponse.clear();
        if (!ReadSmtpResponse([&](std::string& responseLine) { return secureSmtp.Receive(responseLine); }, smtpResponse))
        {
            return false;
        }

        const int code = GetSmtpCode(smtpResponse);
        for (const int ok : okCodes)
        {
            if (code == ok)
            {
                return true;
            }
        }

        return false;
    };

    if (!sendSecureSmtp("EHLO " + username, {250}))
    {
        error = "SMTP EHLO after STARTTLS failed: " + JoinSmtpLines(smtpResponse);
        return false;
    }

    std::string authPayload;
    if (!BuildSmtpPlainAuthPayload(username, password, authPayload))
    {
        error = "SMTP AUTH payload build failed";
        return false;
    }

    if (!sendSecureSmtp("AUTH PLAIN " + authPayload, {235}))
    {
        error = "SMTP AUTH failed: " + JoinSmtpLines(smtpResponse);
        return false;
    }

    return true;
}

bool MailWorker::loginImap(const std::string& host,
                           uint16_t port,
                           const std::string& username,
                           const std::string& password,
                           std::string& error)
{
    if (!m_connector->Connect(host, port, m_imapConnection))
    {
        error = "Failed to connect to IMAP server";
        return false;
    }

    std::string banner;
    if (!m_imapConnection->Receive(banner))
    {
        m_imapConnection->Close();
        m_imapConnection.reset();
        error = "IMAP banner not received";
        return false;
    }

    const std::string tlsTag = nextTag();
	if (!m_imapConnection->Send(tlsTag + " STARTTLS\r\n"))
	{
		m_imapConnection->Close();
		m_imapConnection.reset();
		error = "IMAP STARTTLS send failed";
		return false;
	}

	std::string tlsResponse;
	if (!m_imapConnection->Receive(tlsResponse) || tlsResponse.find(tlsTag + " OK") == std::string::npos)
	{
		m_imapConnection->Close();
		m_imapConnection.reset();
		error = "IMAP STARTTLS rejected: " + tlsResponse;
		return false;
	}

	m_imapSecureConnection = std::make_unique<ClientSecureChannel>(*m_imapConnection);
	if (!m_imapSecureConnection->StartTLS())
	{
		m_imapSecureConnection.reset();
		m_imapConnection->Close();
		m_imapConnection.reset();
		error = "IMAP TLS handshake failed";
		return false;
	}

	const std::string tag = nextTag();
	if (!m_imapSecureConnection->Send(tag + " LOGIN " + QuoteImap(username) + " " + QuoteImap(password) + "\r\n"))
	{
		m_imapSecureConnection.reset();
		m_imapConnection->Close();
		m_imapConnection.reset();
		error = "IMAP login send failed";
		return false;
	}

	while (true)
	{
		std::string line;
		if (!m_imapSecureConnection->Receive(line))
		{
			m_imapSecureConnection.reset();
			m_imapConnection->Close();
			m_imapConnection.reset();
			error = "IMAP login receive failed";
			return false;
		}

		if (line.rfind(tag + " OK", 0) == 0)
		{
			break;
		}

		if (line.rfind(tag + " NO", 0) == 0 || line.rfind(tag + " BAD", 0) == 0)
		{
			m_imapSecureConnection.reset();
			m_imapConnection->Close();
			m_imapConnection.reset();
			error = "IMAP login failed (server returned NO/BAD)";
			return false;
		}
	}

	return true;
}

bool MailWorker::reconnectSession(std::string& error)
{
    if (m_smtpHost.empty() || m_smtpPort == 0 || m_smtpUsername.empty() || m_imapHost.empty() || m_imapPort == 0)
    {
        error = "Reconnection settings are not initialized";
        return false;
    }

    if (!m_connector)
    {
        m_connector = std::make_unique<SocketConnector>();
        m_connector->Initialize(m_ioContext);
    }

    uint32_t attempt = 0;
    while (!m_stopRequested.load(std::memory_order_relaxed))
    {
        ++attempt;
        reportReconnectState(true, attempt, "Reconnecting...");

        if (m_smtpConnection)
        {
            m_smtpConnection->Close();
            m_smtpConnection.reset();
        }

        if (m_imapConnection)
        {
			m_imapSecureConnection.reset();
            m_imapConnection->Close();
            m_imapConnection.reset();
        }

        std::unique_ptr<SocketConnection> smtpConn;
        if (!m_connector->Connect(m_smtpHost, m_smtpPort, smtpConn))
        {
            error = "Failed to reconnect to SMTP";
        }
        else if (!authenticateSmtp(*smtpConn, m_smtpUsername, m_smtpPassword, error))
        {
            smtpConn->Close();
        }
        else
        {
            smtpConn->Close();

            if (loginImap(m_imapHost, m_imapPort, m_imapUsername, m_imapPassword, error))
            {
                m_tagCounter = 0;
                m_currentFolder.clear();
                reportReconnectState(false, attempt, "Reconnected");
                return true;
            }
        }

        reportReconnectState(true, attempt, error.empty() ? "Reconnect failed" : error);
        if (!WaitForReconnectDelay(m_stopRequested, std::chrono::seconds(2)))
        {
            break;
        }
    }

    error = "Reconnect canceled";
    reportReconnectState(false, 0, error);
    return false;
}

bool MailWorker::ensureImapConnected(std::string& error)
{
    if (m_imapConnection)
    {
        return true;
    }

    return reconnectSession(error);
}

bool MailWorker::ensureMailboxSelected(const std::string& folderName, std::string& error)
{
    if (m_currentFolder == folderName)
    {
        return true;
    }

    auto [selectOk, _] = imapExchange("SELECT " + QuoteImap(folderName));
    if (!selectOk)
    {
        error = "SELECT " + folderName + " failed";
        return false;
    }

    m_currentFolder = folderName;
    return true;
}

bool MailWorker::ensureInboxSelected(std::string& error)
{
    return ensureMailboxSelected("INBOX", error);
}

std::vector<std::string> MailWorker::fetchFlagsBySeq(int64_t mailId)
{
    auto [ok, lines] = imapExchange("FETCH " + std::to_string(mailId) + " (FLAGS)");
    if (!ok)
    {
        return {};
    }
    return lines;
}

bool MailWorker::fetchMessagePayloadBySeq(int64_t mailId, std::string& rawEmail)
{
    auto tryFetch = [&](const std::string& fetchItems) -> bool
    {
        auto [ok, fetchedLines] = imapExchange("FETCH " + std::to_string(mailId) + " (" + fetchItems + ")");
        if (!ok)
        {
            return false;
        }

        const std::string candidate = ExtractRfc822Payload(fetchedLines);
        if (candidate.empty())
        {
            return false;
        }

        rawEmail = candidate;
        return true;
    };

    return tryFetch("RFC822") || tryFetch("BODY[]") || tryFetch("BODY.PEEK[]");
}

void MailWorker::reportReconnectState(bool reconnecting, uint32_t attempt, const std::string& message)
{
    m_onResult(ReconnectStateResult{reconnecting, attempt, message});
}

void MailWorker::handleConnect(const ConnectCommand& cmd)
{
    m_connector = std::make_unique<SocketConnector>();
    m_connector->Initialize(m_ioContext);

    m_smtpHost = cmd.SMTPhost;
    m_smtpPort = cmd.SMTPport;
    m_smtpUsername = cmd.username;
    m_smtpPassword = cmd.password;

    m_imapHost = cmd.IMAPhost;
    m_imapPort = cmd.IMAPport;
    m_imapUsername = cmd.username;
    m_imapPassword = cmd.password;

    std::string reconnectError;
    if (!reconnectSession(reconnectError))
    {
        m_onResult(ConnectResult{false, reconnectError});
        return;
    }

    m_tagCounter = 0;
    m_currentFolder.clear();
    m_onResult(ConnectResult{true, {}});
}

void MailWorker::handleDisconnect(const DisconnectCommand&)
{
    if (m_imapConnection)
    {
        imapExchange("LOGOUT");
		m_imapSecureConnection.reset();
        m_imapConnection->Close();
        m_imapConnection.reset();
    }

    if (m_smtpConnection)
    {
        m_smtpConnection->Close();
        m_smtpConnection.reset();
    }

    m_connector.reset();
    m_currentFolder.clear();
    m_tagCounter = 0;
    m_smtpUsername.clear();
    m_smtpPassword.clear();
    m_smtpHost.clear();
    m_smtpPort = 0;
    m_imapUsername.clear();
    m_imapPassword.clear();
    m_imapHost.clear();
    m_imapPort = 0;
    reportReconnectState(false, 0, "");

    m_onResult(DisconnectResult{true, {}});
}

void MailWorker::handleSendMail(const SendMailCommand& cmd)
{
    if (m_smtpHost.empty() || !m_connector || m_smtpUsername.empty())
    {
        m_onResult(SendMailResult{false, "Not connected - call connect first"});
        return;
    }

    std::unique_ptr<SocketConnection> smtpConn;
    if (!m_connector->Connect(m_smtpHost, m_smtpPort, smtpConn))
    {
        std::string reconnectError;
        if (!reconnectSession(reconnectError) || !m_connector->Connect(m_smtpHost, m_smtpPort, smtpConn))
        {
            m_onResult(SendMailResult{false, "Failed to reconnect to SMTP"});
            return;
        }
    }

    std::vector<std::string> responseLines;
    if (!ReadSmtpResponse([&](std::string& line) { return smtpConn->Receive(line); }, responseLines) ||
        GetSmtpCode(responseLines) != 220)
    {
        m_onResult(SendMailResult{false, "SMTP greeting failed"});
        return;
    }

    auto sendPlain = [&](const std::string& line, std::initializer_list<int> okCodes) -> bool
    {
        if (!smtpConn->Send(line + "\r\n"))
        {
            return false;
        }

        responseLines.clear();
        if (!ReadSmtpResponse([&](std::string& response) { return smtpConn->Receive(response); }, responseLines))
        {
            return false;
        }

        const int code = GetSmtpCode(responseLines);
        for (const int ok : okCodes)
        {
            if (code == ok)
            {
                return true;
            }
        }

        return false;
    };

    if (!sendPlain("EHLO " + m_smtpUsername, {250}))
    {
        m_onResult(SendMailResult{false, "EHLO failed: " + JoinSmtpLines(responseLines)});
        return;
    }

    if (!sendPlain("STARTTLS", {220}))
    {
        m_onResult(SendMailResult{false, "STARTTLS failed: " + JoinSmtpLines(responseLines)});
        return;
    }

    ClientSecureChannel secureSmtp(*smtpConn);
    if (!secureSmtp.StartTLS())
    {
        m_onResult(SendMailResult{false, "TLS handshake failed"});
        return;
    }

    auto sendLine = [&](const std::string& line, std::initializer_list<int> okCodes) -> bool
    {
        if (!secureSmtp.Send(line + "\r\n"))
        {
            return false;
        }

        responseLines.clear();
        if (!ReadSmtpResponse([&](std::string& response) { return secureSmtp.Receive(response); }, responseLines))
        {
            return false;
        }

        const int code = GetSmtpCode(responseLines);
        for (const int ok : okCodes)
        {
            if (code == ok)
            {
                return true;
            }
        }

        return false;
    };

    if (!sendLine("EHLO " + m_smtpUsername, {250}))
    {
        m_onResult(SendMailResult{false, "EHLO after STARTTLS failed: " + JoinSmtpLines(responseLines)});
        return;
    }

    std::string authPayload;
    if (!BuildSmtpPlainAuthPayload(m_smtpUsername, m_smtpPassword, authPayload) ||
        !sendLine("AUTH PLAIN " + authPayload, {235}))
    {
        m_onResult(SendMailResult{false, "AUTH failed: " + JoinSmtpLines(responseLines)});
        return;
    }

    if (!sendLine("MAIL FROM:<" + cmd.sender + ">", {250}))
    {
        m_onResult(SendMailResult{false, "MAIL FROM failed"});
        return;
    }

    const std::vector<std::string> smtpRecipients = CollectRecipients(cmd);
    if (smtpRecipients.empty())
    {
        m_onResult(SendMailResult{false, "No recipients provided"});
        return;
    }

    for (const auto& rcpt : smtpRecipients)
    {
        if (!sendLine("RCPT TO:<" + rcpt + ">", {250, 251}))
        {
            m_onResult(SendMailResult{false, "RCPT TO failed"});
            return;
        }
    }

    if (!sendLine("DATA", {354}))
    {
        m_onResult(SendMailResult{false, "DATA failed"});
        return;
    }

    auto sendDataLine = [&](const std::string& line) -> bool
    {
        return secureSmtp.Send(line + "\r\n");
    };

    SmtpClient::Email email;
    email.sender = cmd.sender;
    email.to = !cmd.to.empty() ? cmd.to : cmd.recipients;
    email.cc = cmd.cc;
    email.bcc = cmd.bcc;
    email.subject = cmd.subject;
    email.plain_text = cmd.body;
    email.html_text = cmd.htmlBody;

    for (const auto& path : cmd.attachments)
    {
        if (path.empty())
        {
            continue;
        }

        const std::string encoded = AttachmentHandler::EncodeFile(path);
        if (encoded.empty())
        {
            m_onResult(SendMailResult{false, "Attachment encode failed: " + path});
            return;
        }

        const std::string fileName = std::filesystem::path(path).filename().string();
        email.AddAttachment(fileName.empty() ? path : fileName, "", encoded);
    }

    SilentLogger mimeLogger;
    std::string mimePayload;
    if (!SmtpClient::MimeBuilder::BuildEmail(email, mimePayload, mimeLogger))
    {
        m_onResult(SendMailResult{false, "Failed to build MIME message"});
        return;
    }

    std::istringstream payloadStream(mimePayload);
    std::string bodyLine;
    while (std::getline(payloadStream, bodyLine))
    {
        if (!bodyLine.empty() && bodyLine.back() == '\r')
        {
            bodyLine.pop_back();
        }

        // SMTP transparency: lines that start with '.' must be escaped.
        if (!bodyLine.empty() && bodyLine.front() == '.')
        {
            bodyLine.insert(bodyLine.begin(), '.');
        }

        if (!sendDataLine(bodyLine))
        {
            m_onResult(SendMailResult{false, "Failed to send MIME payload"});
            return;
        }
    }

    if (!sendDataLine("."))
    {
        m_onResult(SendMailResult{false, "Failed to terminate DATA"});
        return;
    }

    responseLines.clear();
    if (!ReadSmtpResponse([&](std::string& response) { return secureSmtp.Receive(response); }, responseLines))
    {
        m_onResult(SendMailResult{false, "Message body transfer failed"});
        return;
    }

    if (GetSmtpCode(responseLines) != 250)
    {
        m_onResult(SendMailResult{false, "Message rejected: " + JoinSmtpLines(responseLines)});
        return;
    }

    // Do not block on QUIT response: some servers close early or respond slowly.
    secureSmtp.Send("QUIT\r\n");
    smtpConn->Close();

    m_onResult(SendMailResult{true, {}});
}

void MailWorker::handleFetchFolders(const FetchFoldersCommand&)
{
    std::string error;
    if (!ensureImapConnected(error))
    {
        m_onResult(FetchFoldersResult{false, error.empty() ? "Not connected" : error, {}});
        return;
    }

    auto [ok, lines] = imapExchange("LIST \"\" \"*\"");
    if (!ok)
    {
        m_onResult(FetchFoldersResult{false, "LIST failed", {}});
        return;
    }

    std::vector<std::string> folders;
    for (const auto& line : lines)
    {
        const auto pos = line.rfind('"');
        if (pos == std::string::npos) continue;

        const auto start = line.rfind('"', pos - 1);
        if (start == std::string::npos) continue;

        folders.push_back(line.substr(start + 1, pos - start - 1));
    }

    m_onResult(FetchFoldersResult{true, {}, folders});
}

void MailWorker::handleFetchMails(const FetchMailsCommand& cmd)
{
    std::string error;
    if (!ensureImapConnected(error))
    {
        m_onResult(FetchMailsResult{false, error.empty() ? "Not connected" : error, {}});
        return;
    }

    if (!ensureMailboxSelected(cmd.folderName, error))
    {
        m_onResult(FetchMailsResult{false, error, {}});
        return;
    }

    const unsigned int from = cmd.offset + 1;
    const unsigned int to = cmd.offset + std::max(1u, cmd.limit);

    const std::string range = std::to_string(from) + ":" + std::to_string(to);
    auto [ok, lines] = imapExchange("FETCH " + range + " (FLAGS RFC822)");

    if (!ok)
    {
        m_onResult(FetchMailsResult{false, "FETCH failed", {}});
        return;
    }

    std::vector<std::size_t> fetchStarts;
    for (std::size_t i = 0; i < lines.size(); ++i)
    {
        if (lines[i].rfind("* ", 0) == 0 && lines[i].find(" FETCH ") != std::string::npos)
        {
            fetchStarts.push_back(i);
        }
    }

    std::vector<MailSummary> mails;
    mails.reserve(fetchStarts.size());

    for (std::size_t k = 0; k < fetchStarts.size(); ++k)
    {
        const std::size_t blockStart = fetchStarts[k];
        const std::size_t blockEnd   = (k + 1 < fetchStarts.size()) ? fetchStarts[k + 1] : lines.size();

        std::vector<std::string> block(lines.begin() + blockStart, lines.begin() + blockEnd);

        const std::size_t fetchPos = block[0].find(" FETCH ");
		if (fetchPos == std::string::npos)
		{
			continue;
		}

        int64_t seqId = 0;
        try 
        { 
            seqId = std::stoll(block[0].substr(2, fetchPos - 2));
        } catch (...) 
        {
            continue; 
        }

        std::string rawEmail = ExtractRfc822Payload(block);
		if (rawEmail.empty())
		{
			continue;
		}

        MailSummary summary;
        summary.mailId = seqId;
        ExtractFlags(block, summary.isSeen, summary.isFlagged);

        SilentLogger mimeLogger;
        SmtpClient::Email parsed;
        if (SmtpClient::MimeParser::ParseEmail(rawEmail, parsed, mimeLogger))
        {
            summary.subject = parsed.subject;
            summary.date = parsed.date;
            summary.preview = BuildPreview(parsed.plain_text, parsed.html_text, 10);
        }
        else
        {
            summary.preview = BuildPreview(rawEmail, {}, 10);
        }

        mails.push_back(std::move(summary));
    }

    m_onResult(FetchMailsResult{true, {}, mails});
}

void MailWorker::handleFetchMail(const FetchMailCommand& cmd)
{
    std::string error;
    if (!ensureImapConnected(error))
    {
        m_onResult(FetchMailResult{false, error.empty() ? "Not connected" : error, {}});
        return;
    }

    std::vector<std::string> lines = fetchFlagsBySeq(cmd.mailId);
    std::string rawEmail;
    if (!fetchMessagePayloadBySeq(cmd.mailId, rawEmail))
    {
        m_onResult(FetchMailResult{false, "FETCH failed or returned empty payload", {}});
        return;
    }

    MailDetails mail;
    mail.mailId = cmd.mailId;
    ExtractFlags(lines, mail.isSeen, mail.isFlagged);

    if (rawEmail.empty())
    {
        m_onResult(FetchMailResult{false, "Empty payload", {}});
        return;
    }

    SilentLogger mimeLogger;
    SmtpClient::Email parsed;
    if (SmtpClient::MimeParser::ParseEmail(rawEmail, parsed, mimeLogger))
    {
        mail.sender = parsed.sender;
        mail.to = parsed.to;
        mail.cc = parsed.cc;
        mail.subject = parsed.subject;
        mail.date = parsed.date;
        mail.messageId = parsed.message_id;
        mail.inReplyTo = parsed.in_reply_to;
        mail.references = parsed.references;
        mail.plainBody = parsed.plain_text;
        mail.htmlBody = parsed.html_text;
        mail.body = !parsed.plain_text.empty() ? parsed.plain_text : parsed.html_text;

        for (const auto& attachment : parsed.attachments)
        {
            mail.attachments.push_back(AttachmentInfo{
                attachment.file_name,
                attachment.mime_type,
                attachment.file_size});
        }

        std::string headers;
        std::string ignoreBody;
        SmtpClient::MimeParser::SplitHeadersAndBody(rawEmail, headers, ignoreBody);

        const std::string bccHeader = SmtpClient::MimeParser::GetHeaderValue(headers, "Bcc:");
        const std::string replyToHeader = SmtpClient::MimeParser::GetHeaderValue(headers, "Reply-To:");
        mail.bcc = SplitAddressList(bccHeader);
        mail.replyTo = SplitAddressList(replyToHeader);

        m_onResult(FetchMailResult{true, {}, std::move(mail)});
    }
    else
    {
        mail.body = rawEmail;
        m_onResult(FetchMailResult{true, {}, std::move(mail)});
    }
}

void MailWorker::handleDeleteMail(const DeleteMailCommand& cmd)
{
    std::string error;
    if (!ensureImapConnected(error))
    {
        m_onResult(MailActionResult{false, error.empty() ? "Not connected" : error});
        return;
    }

    if (!ensureInboxSelected(error))
    {
        m_onResult(MailActionResult{false, error});
        return;
    }

    auto [storeOk, _1] = imapExchange("STORE " + std::to_string(cmd.mailId) + " +FLAGS (\\Deleted)");
    if (!storeOk)
    {
        m_onResult(MailActionResult{false, "STORE failed"});
        return;
    }

    auto [expungeOk, _2] = imapExchange("EXPUNGE");
    m_onResult(MailActionResult{expungeOk, expungeOk ? "" : "EXPUNGE failed"});
}

void MailWorker::handleMarkMailRead(const MarkMailReadCommand& cmd)
{
    std::string error;
    if (!ensureImapConnected(error))
    {
        m_onResult(MailActionResult{false, error.empty() ? "Not connected" : error});
        return;
    }

    if (!ensureInboxSelected(error))
    {
        m_onResult(MailActionResult{false, error});
        return;
    }

    auto [ok, _] = imapExchange("STORE " + std::to_string(cmd.mailId) + " +FLAGS (\\Seen)");
    m_onResult(MailActionResult{ok, ok ? "" : "STORE failed"});
}

void MailWorker::handleMoveMail(const MoveMailCommand& cmd)
{
    std::string error;
    if (!ensureImapConnected(error))
    {
        m_onResult(MailActionResult{false, error.empty() ? "Not connected" : error});
        return;
    }

    if (!ensureInboxSelected(error))
    {
        m_onResult(MailActionResult{false, error});
        return;
    }

    if (cmd.targetFolder.empty())
    {
        m_onResult(MailActionResult{false, "Target folder is empty"});
        return;
    }

    auto [copyOk, _1] = imapExchange("COPY " + std::to_string(cmd.mailId) + " " + QuoteImap(cmd.targetFolder));
    if (!copyOk)
    {
        m_onResult(MailActionResult{false, "COPY failed"});
        return;
    }

    auto [storeOk, _2] = imapExchange("STORE " + std::to_string(cmd.mailId) + " +FLAGS (\\Deleted)");
    if (!storeOk)
    {
        m_onResult(MailActionResult{false, "STORE failed"});
        return;
    }

    auto [expungeOk, _3] = imapExchange("EXPUNGE");
    m_onResult(MailActionResult{expungeOk, expungeOk ? "" : "EXPUNGE failed"});
}

void MailWorker::handleDownloadAttachment(const DownloadAttachmentCommand& cmd)
{
    std::string error;
    if (!ensureImapConnected(error))
    {
        m_onResult(DownloadAttachmentResult{false, error.empty() ? "Not connected" : error, {}, {}, 0});
        return;
    }

    if (cmd.attachmentIndex == 0)
    {
        m_onResult(DownloadAttachmentResult{false, "Attachment index is 1-based and must be > 0", {}, {}, 0});
        return;
    }

    if (!ensureInboxSelected(error))
    {
        m_onResult(DownloadAttachmentResult{false, error, {}, {}, 0});
        return;
    }

    std::string rawEmail;
    if (!fetchMessagePayloadBySeq(cmd.mailId, rawEmail))
    {
        m_onResult(DownloadAttachmentResult{false, "FETCH failed", {}, {}, 0});
        return;
    }

    if (rawEmail.empty())
    {
        m_onResult(DownloadAttachmentResult{false, "Empty RFC822 payload", {}, {}, 0});
        return;
    }

    SilentLogger mimeLogger;
    SmtpClient::Email parsed;
    if (!SmtpClient::MimeParser::ParseEmail(rawEmail, parsed, mimeLogger))
    {
        m_onResult(DownloadAttachmentResult{false, "Failed to parse MIME message", {}, {}, 0});
        return;
    }

    if (parsed.attachments.empty())
    {
        m_onResult(DownloadAttachmentResult{false, "No attachments in this message", {}, {}, 0});
        return;
    }

    if (cmd.attachmentIndex > parsed.attachments.size())
    {
        m_onResult(DownloadAttachmentResult{false, "Attachment index out of range", {}, {}, 0});
        return;
    }

    const auto& attachment = parsed.attachments[cmd.attachmentIndex - 1];
    std::vector<uint8_t> bytes;
    if (!TryDecodeAttachmentBytes(attachment, bytes))
    {
        m_onResult(DownloadAttachmentResult{false, "Attachment payload is empty", {}, {}, 0});
        return;
    }

    const std::string safeName = SanitizeAttachmentFileName(attachment.file_name);
    std::filesystem::path outputPath;
    if (cmd.outputPath.empty())
    {
        outputPath = std::filesystem::path(".tmp") / "attachments" / safeName;
    }
    else
    {
        std::filesystem::path requestedPath(cmd.outputPath);

        std::error_code statEc;
        const bool exists = std::filesystem::exists(requestedPath, statEc);
        const bool isDirectory = exists && std::filesystem::is_directory(requestedPath, statEc);
        const bool trailingSeparator = !cmd.outputPath.empty() &&
            (cmd.outputPath.back() == '/' || cmd.outputPath.back() == '\\');

        if (isDirectory || trailingSeparator || !requestedPath.has_filename())
        {
            outputPath = requestedPath / safeName;
        }
        else
        {
            outputPath = requestedPath;
        }
    }

    const std::filesystem::path parent = outputPath.parent_path();
    if (!parent.empty())
    {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
        if (ec)
        {
            m_onResult(DownloadAttachmentResult{false, "Failed to create output directory", {}, {}, 0});
            return;
        }
    }

    std::ofstream file(outputPath, std::ios::binary | std::ios::trunc);
    if (!file)
    {
        m_onResult(DownloadAttachmentResult{false, "Failed to open output file", {}, {}, 0});
        return;
    }

    if (!bytes.empty())
    {
        file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }

    if (!file.good())
    {
        m_onResult(DownloadAttachmentResult{false, "Failed to write output file", {}, {}, 0});
        return;
    }

    m_onResult(DownloadAttachmentResult{true, {}, outputPath.string(), attachment.file_name, bytes.size()});
}

void MailWorker::handleCreateFolder(const CreateFolderCommand& cmd)
{
    std::string error;
    if (!ensureImapConnected(error))
    {
        m_onResult(MailActionResult{false, error.empty() ? "Not connected" : error});
        return;
    }

    auto [ok, _] = imapExchange("CREATE " + QuoteImap(cmd.folderName));
    m_onResult(MailActionResult{ok, ok ? "" : "CREATE failed"});
}

void MailWorker::handleDeleteFolder(const DeleteFolderCommand& cmd)
{
    std::string error;
    if (!ensureImapConnected(error))
    {
        m_onResult(MailActionResult{false, error.empty() ? "Not connected" : error});
        return;
    }

    auto [ok, _] = imapExchange("DELETE " + QuoteImap(cmd.folderName));
    if (ok && m_currentFolder == cmd.folderName)
    {
        m_currentFolder.clear();
    }
    m_onResult(MailActionResult{ok, ok ? "" : "DELETE failed"});
}

void MailWorker::handleAddLabel(const AddLabelCommand& cmd)
{
    std::string error;
    if (!ensureImapConnected(error))
    {
        m_onResult(MailActionResult{false, error.empty() ? "Not connected" : error});
        return;
    }

    if (!ensureInboxSelected(error))
    {
        m_onResult(MailActionResult{false, error});
        return;
    }

    const std::string flag = NormalizeFlag(cmd.label);
    auto [ok, _] = imapExchange("STORE " + std::to_string(cmd.mailId) + " +FLAGS (" + flag + ")");
    m_onResult(MailActionResult{ok, ok ? "" : "STORE failed"});
}

void MailWorker::handleRemoveLabel(const RemoveLabelCommand& cmd)
{
    std::string error;
    if (!ensureImapConnected(error))
    {
        m_onResult(MailActionResult{false, error.empty() ? "Not connected" : error});
        return;
    }

    if (!ensureInboxSelected(error))
    {
        m_onResult(MailActionResult{false, error});
        return;
    }

    const std::string flag = NormalizeFlag(cmd.label);
    auto [ok, _] = imapExchange("STORE " + std::to_string(cmd.mailId) + " -FLAGS (" + flag + ")");
    m_onResult(MailActionResult{ok, ok ? "" : "STORE failed"});
}

void MailWorker::handleMarkMailStarred(const MarkMailStarredCommand& cmd)
{
    std::string error;
    if (!ensureImapConnected(error))
    {
        m_onResult(MailActionResult{false, error.empty() ? "Not connected" : error});
        return;
    }

    if (!ensureInboxSelected(error))
    {
        m_onResult(MailActionResult{false, error});
        return;
    }

    const std::string targetFolder = cmd.starred ? "Starred" : "INBOX";

    if (!cmd.starred && !IsStarredFolder(m_currentFolder))
    {
        // If current folder is not Starred, message is already treated as unstarred.
        m_onResult(MailActionResult{true, {}});
        return;
    }

    if (cmd.starred)
    {
        // Ensure the Starred folder exists; ignore errors if it already exists.
        imapExchange("CREATE " + QuoteImap(targetFolder));
    }

    auto [copyOk, _1] = imapExchange("COPY " + std::to_string(cmd.mailId) + " " + QuoteImap(targetFolder));
    if (!copyOk)
    {
        m_onResult(MailActionResult{false, "COPY failed"});
        return;
    }

    auto [storeOk, _2] = imapExchange("STORE " + std::to_string(cmd.mailId) + " +FLAGS (\\Deleted)");
    if (!storeOk)
    {
        m_onResult(MailActionResult{false, "STORE failed"});
        return;
    }

    auto [expungeOk, _3] = imapExchange("EXPUNGE");
    m_onResult(MailActionResult{expungeOk, expungeOk ? "" : "EXPUNGE failed"});
}

void MailWorker::handleMarkMailImportant(const MarkMailImportantCommand& cmd)
{
    std::string error;
    if (!ensureImapConnected(error))
    {
        m_onResult(MailActionResult{false, error.empty() ? "Not connected" : error});
        return;
    }

    if (!ensureInboxSelected(error))
    {
        m_onResult(MailActionResult{false, error});
        return;
    }

    const std::string flags = cmd.important ? "+FLAGS (\\Flagged)" : "-FLAGS (\\Flagged)";
    auto [ok, _] = imapExchange("STORE " + std::to_string(cmd.mailId) + " " + flags);
    m_onResult(MailActionResult{ok, ok ? "" : "STORE failed"});
}

