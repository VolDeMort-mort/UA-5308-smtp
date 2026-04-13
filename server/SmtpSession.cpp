#include "SmtpSession.hpp"

#include "SmtpParser.hpp"
#include "SmtpResponse.hpp"
#include "MimeParser.h"
#include "Email.h"
#include "Entity/Recipient.h"
#include <chrono>
#include <ctime>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <random>

SmtpSession::SmtpSession(std::string domain, MessageRepository* message_repo, UserRepository* user_repo, Logger* logger)
    : m_domain(std::move(domain)),
      m_message_repo(message_repo),
      m_user_repo(user_repo),
      m_logger(logger)
{
}

std::string SmtpSession::ExtractUsername(const std::string& email)
{
    const auto pos = email.find('@');
    if (pos == std::string::npos)
        return email;
    return email.substr(0, pos);
}

namespace
{
    std::string CurrentUtcTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::gmtime(&t));
        return buf;
    }


    std::string GenerateMailFilename()
    {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

        std::random_device               rd;
        std::mt19937                     gen(rd());
        std::uniform_int_distribution<>  dist(0, 15);
        const char* hex = "0123456789abcdef";
        std::string rnd;
        for (int i = 0; i < 16; ++i) { rnd += hex[dist(gen)]; }

        return std::to_string(ms) + "_" + rnd + ".eml";
    }


    std::string WriteMailToFile(const std::string& mail_dir, const std::string& body)
    {
        std::error_code ec;
        std::filesystem::create_directories(mail_dir, ec);
        if (ec) return {};

        std::string path = mail_dir + "/" + GenerateMailFilename();
        std::ofstream ofs(path, std::ios::binary);
        if (!ofs) return {};

        ofs.write(body.data(), static_cast<std::streamsize>(body.size()));
        return ofs ? path : std::string{};
    }
} 

bool SmtpSession::SaveMessage()
{
    if (m_recipients.empty())
        return false;


    SmtpClient::Email parsed_email;
    bool mime_ok = false;
    if (m_logger)
        mime_ok = SmtpClient::MimeParser::ParseEmail(m_body, parsed_email, *m_logger);



    std::string mime_structure;
    if (mime_ok)
    {
        if (!parsed_email.attachments.empty())
        {
            mime_structure = "multipart/mixed; parts=" +
                std::to_string(1 + static_cast<int>(parsed_email.attachments.size())) +
                "; attachments=" + std::to_string(parsed_email.attachments.size());
        }
        else if (parsed_email.HasHtml())
        {
            mime_structure = "multipart/alternative; parts=2; attachments=0";
        }
        else
        {
            mime_structure = "text/plain";
        }
    }

    bool any_saved = false;

    for (const auto& recipient_addr : m_recipients)
    {
        std::string username = ExtractUsername(recipient_addr);

        auto user = m_user_repo->findByUsername(username);

        if (!user)
        {
            User new_user;
            new_user.username      = username;
            new_user.password_hash = "smtp_auto";

            if (!m_user_repo->registerUser(new_user, ""))
                continue;

            user = m_user_repo->findByUsername(username);
        }

        if (!user || !user->id)
            continue;

        auto inbox = m_message_repo->findFolderByName(user->id.value(), "INBOX");
        if (!inbox || !inbox->id)
            continue;


        std::string mail_dir = "mailstore/" + username;
        std::string file_path = WriteMailToFile(mail_dir, m_body);
        if (file_path.empty())
        {
            if (m_logger)
                m_logger->Log(PROD, "SaveMessage: failed to write mail file for user " + username);
            continue;
        }


        Message msg;
        msg.user_id       = user->id.value();
        msg.raw_file_path = file_path;
        msg.size_bytes    = static_cast<int64_t>(m_body.size());
        msg.internal_date = CurrentUtcTimestamp();

        msg.from_address = (mime_ok && !parsed_email.sender.empty())
                               ? parsed_email.sender
                               : m_sender;

        if (mime_ok)
        {
            if (!parsed_email.subject.empty())
                msg.subject = parsed_email.subject;

            if (!parsed_email.message_id.empty())
                msg.message_id_header = parsed_email.message_id;

            if (!parsed_email.in_reply_to.empty())
                msg.in_reply_to = parsed_email.in_reply_to;

            if (!parsed_email.references.empty())
                msg.references_header = parsed_email.references;

            if (!parsed_email.date.empty())
                msg.date_header = parsed_email.date;
        }

        if (!mime_structure.empty())
            msg.mime_structure = mime_structure;


        if (!m_message_repo->deliver(msg, inbox->id.value()))
            continue;
		

        Recipient rec;
        rec.message_id = msg.id.value();
        rec.address    = recipient_addr;
        rec.type       = RecipientType::To;
        m_message_repo->addRecipient(rec);

        if (mime_ok)
        {
            auto add_mime_recipients = [&](const std::vector<std::string>& addrs, RecipientType type)
            {
                for (const auto& addr : addrs)
                {
                    if (addr.empty())
                        continue;
                    if (type == RecipientType::To && addr == recipient_addr)
                        continue; // already added above

                    Recipient mime_rec;
                    mime_rec.message_id = msg.id.value();
                    mime_rec.address    = addr;
                    mime_rec.type       = type;
                    m_message_repo->addRecipient(mime_rec);
                }
            };

            add_mime_recipients(parsed_email.to, RecipientType::To);
            add_mime_recipients(parsed_email.cc, RecipientType::Cc);
            add_mime_recipients(parsed_email.bcc, RecipientType::Bcc);
        }

        any_saved = true;
    }

    return any_saved;
}

std::string SmtpSession::Greeting() const
{
    return SmtpResponse::ServiceReady(m_domain);
}

bool SmtpSession::IsClosed() const noexcept
{
    return m_state == SmtpState::CLOSED;
}

void SmtpSession::ResetMessage()
{
    m_sender.clear();
    m_recipients.clear();
    m_body.clear();
}

void SmtpSession::ResetToHelo()
{
	m_state = SmtpState::WAIT_HELO;
	m_authenticated = false;
	m_auth_username.clear();
	ResetMessage();
}

std::string SmtpSession::ProcessLine(const std::string& line)
{
    constexpr size_t MAX_SMTP_LINE = 512;

    if (line.size() > MAX_SMTP_LINE)
        return SmtpResponse::SyntaxError();

    if (m_state == SmtpState::AUTH_WAIT_USER || m_state == SmtpState::AUTH_WAIT_PASS) 
        return HandleAuthLine(line);

    if (m_state == SmtpState::RECEIVING_DATA)
    {
        if (line == ".")
        {	
			if(!SaveMessage())
			{
				ResetMessage();
				m_state = SmtpState::WAIT_MAIL;
				return SmtpResponse::MessageStorageFailed();
			}
            m_state = SmtpState::WAIT_MAIL;

            ResetMessage();

            return SmtpResponse::Ok();
        }

        m_body += line + "\n";

        if (m_body.size() > MAX_MESSAGE_SIZE)
        {
            m_state = SmtpState::WAIT_MAIL;

            ResetMessage();

            return SmtpResponse::MessageTooLarge();
        }

        return {};
    }

    SmtpCommand command = SmtpParser::Parse(line);

    switch (command.type)
    {
        case SmtpCommandType::HELO:
        case SmtpCommandType::EHLO:
            return HandleHelo(command);

        case SmtpCommandType::AUTH:
			return HandleAuth(command);

        case SmtpCommandType::MAIL:
            return HandleMail(command);

        case SmtpCommandType::RCPT:
            return HandleRcpt(command);

        case SmtpCommandType::DATA:
            return HandleData(command);

        case SmtpCommandType::RSET:
            return HandleRset();

        case SmtpCommandType::NOOP:
            return HandleNoop();

        case SmtpCommandType::QUIT:
            return HandleQuit();

        case SmtpCommandType::UNKNOWN:
            return SmtpResponse::NotImplemented();

		case SmtpCommandType::STARTTLS:
			return HandleStartTLS();
    }

    return SmtpResponse::SyntaxError();
}

std::string SmtpSession::HandleHelo(const SmtpCommand& command)
{
	ResetToHelo();

	m_state = SmtpState::WAIT_MAIL;

	if (command.type == SmtpCommandType::EHLO) 
        return SmtpResponse::Ehlo(m_domain, m_secure);

	return SmtpResponse::Hello(m_domain);
}

std::string SmtpSession::HandleMail(const SmtpCommand& command)
{
    if (m_state != SmtpState::WAIT_MAIL)
        return SmtpResponse::BadSequence();

    if (!m_authenticated) return SmtpResponse::AuthRequired();

    m_sender = ExtractUsername(command.argument);

    m_state = SmtpState::WAIT_RCPT;

    return SmtpResponse::Ok();
}

std::string SmtpSession::HandleRcpt(const SmtpCommand& command)
{
    if (m_state != SmtpState::WAIT_RCPT)
        return SmtpResponse::BadSequence();

    m_recipients.push_back(ExtractUsername(command.argument));

    return SmtpResponse::Ok();
}

std::string SmtpSession::HandleData(const SmtpCommand&)
{
    if (m_state != SmtpState::WAIT_RCPT ||
        m_recipients.empty())
    {
        return SmtpResponse::BadSequence();
    }

    m_state = SmtpState::RECEIVING_DATA;

    return SmtpResponse::StartMailInput();
}

std::string SmtpSession::HandleRset()
{
    ResetMessage();

    m_state = SmtpState::WAIT_MAIL;

    return SmtpResponse::Ok();
}

std::string SmtpSession::HandleNoop()
{
    return SmtpResponse::Ok();
}

std::string SmtpSession::HandleQuit()
{
    m_state = SmtpState::CLOSED;

    return SmtpResponse::Closing();
}

std::string SmtpSession::HandleStartTLS()
{
	if (m_state != SmtpState::WAIT_MAIL)
	{
		return SmtpResponse::BadSequence();
	}

     m_state = SmtpState::STARTTLS;

     return SmtpResponse::StartTls();
}

std::string SmtpSession::HandleAuth(const SmtpCommand& command)
{
	if (m_state != SmtpState::WAIT_MAIL) return SmtpResponse::BadSequence();

	if (m_authenticated) return SmtpResponse::BadSequence();

	if (!m_secure) return SmtpResponse::EncryptionRequired();

	auto sp = command.argument.find(' ');
	std::string mechanism = command.argument.substr(0, sp);
	std::string initial_response = (sp == std::string::npos) ? "" : command.argument.substr(sp + 1);


	if (mechanism == "LOGIN")
	{
		m_state = SmtpState::AUTH_WAIT_USER;
		const std::string label = "Username:";
		std::vector<uint8_t> bytes(label.begin(), label.end());
		return SmtpResponse::AuthChallenge(Base64Encoder::EncodeBase64(bytes));
	}

	if (mechanism == "PLAIN")
	{
		if (initial_response.empty())
		{

			m_state = SmtpState::AUTH_WAIT_PASS;
			return SmtpResponse::AuthChallenge("");
		}

		std::vector<uint8_t> decoded;
		try
		{
			decoded = Base64Decoder::DecodeBase64(initial_response);
		}
		catch (const std::exception&)
		{
			return SmtpResponse::AuthFailed();
		}

		if (decoded.empty()) return SmtpResponse::AuthFailed();


		std::string blob(reinterpret_cast<char*>(decoded.data()), decoded.size());

		auto first_nul = blob.find('\0');
		auto second_nul = (first_nul == std::string::npos) ? std::string::npos : blob.find('\0', first_nul + 1);

		if (first_nul == std::string::npos || second_nul == std::string::npos) return SmtpResponse::AuthFailed();

		std::string username = ExtractUsername(blob.substr(first_nul + 1, second_nul - first_nul - 1));
		std::string password = blob.substr(second_nul + 1);

		if (!m_user_repo->authorize(username, password)) return SmtpResponse::AuthFailed();

		m_authenticated = true;
		return SmtpResponse::AuthSucceeded();
	}

	return SmtpResponse::UnrecognizedAuthMech();
}

std::string SmtpSession::HandleAuthLine(const std::string& line)
{
	std::vector<uint8_t> decoded;
	try
	{
		decoded = Base64Decoder::DecodeBase64(line);
	}
	catch (const std::exception&)
	{
		m_state = SmtpState::WAIT_MAIL;
		m_auth_username.clear();
		return SmtpResponse::AuthFailed();
	}

	if (decoded.empty() && !line.empty())
	{
		m_state = SmtpState::WAIT_MAIL;
		m_auth_username.clear();
		return SmtpResponse::AuthFailed();
	}

	if (m_state == SmtpState::AUTH_WAIT_USER)
	{
		std::string name(reinterpret_cast<char*>(decoded.data()), decoded.size());
		m_auth_username = ExtractUsername(name);

		m_state = SmtpState::AUTH_WAIT_PASS;

		const std::string label = "Password:";
		std::vector<uint8_t> bytes(label.begin(), label.end());
		return SmtpResponse::AuthChallenge(Base64Encoder::EncodeBase64(bytes));
	}

	if (m_state == SmtpState::AUTH_WAIT_PASS)
	{
		std::string value(reinterpret_cast<char*>(decoded.data()), decoded.size());

		if (m_auth_username.empty())
		{
			auto first_nul = value.find('\0');
			auto second_nul = (first_nul == std::string::npos) ? std::string::npos : value.find('\0', first_nul + 1);

			if (first_nul == std::string::npos || second_nul == std::string::npos)
			{
				m_state = SmtpState::WAIT_MAIL;
				return SmtpResponse::AuthFailed();
			}

			std::string username = ExtractUsername(value.substr(first_nul + 1, second_nul - first_nul - 1));
			std::string password = value.substr(second_nul + 1);

			m_state = SmtpState::WAIT_MAIL;

			if (!m_user_repo->authorize(username, password)) return SmtpResponse::AuthFailed();
		}
		else
		{

			std::string username = std::move(m_auth_username);
			m_auth_username.clear();
			m_state = SmtpState::WAIT_MAIL;

			if (!m_user_repo->authorize(username, value)) return SmtpResponse::AuthFailed();
		}

		m_authenticated = true;
		return SmtpResponse::AuthSucceeded();
	}

	return SmtpResponse::SyntaxError();
}