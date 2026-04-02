// Headless interactive console tool used when Qt6 is not available.
// It provides a simple REPL for manual SMTP/IMAP testing through MailWorker.

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>

#include "CommandQueue.hpp"
#include "MailCommand.hpp"
#include "MailResult.hpp"
#include "MailWorker.hpp"

namespace
{
struct ResultInbox
{
    void push(MailResult result)
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push_back(std::move(result));
        }
        condition.notify_one();
    }

    std::optional<MailResult> waitPop(int timeoutMs)
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (!condition.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this] { return !queue.empty(); }))
        {
            return std::nullopt;
        }

        MailResult result = std::move(queue.front());
        queue.erase(queue.begin());
        return result;
    }

    std::mutex mutex;
    std::condition_variable condition;
    std::vector<MailResult> queue;
};

std::vector<std::string> splitArgs(const std::string& line)
{
    std::vector<std::string> out;
    std::string current;
    bool inQuotes = false;
    bool tokenStarted = false;

    for (std::size_t i = 0; i < line.size(); ++i)
    {
        const char c = line[i];

        if (c == '"')
        {
            inQuotes = !inQuotes;
            tokenStarted = true;
            continue;
        }

        if (c == '\\' && i + 1 < line.size() && line[i + 1] == '"')
        {
            current.push_back('"');
            tokenStarted = true;
            ++i;
            continue;
        }

        if (!inQuotes && (c == ' ' || c == '\t'))
        {
            if (tokenStarted)
            {
                out.push_back(current);
                current.clear();
                tokenStarted = false;
            }
            continue;
        }

        current.push_back(c);
        tokenStarted = true;
    }

    if (tokenStarted)
    {
        out.push_back(current);
    }

    return out;
}

std::vector<std::string> splitBySemicolon(const std::string& text)
{
    std::vector<std::string> out;
    std::string current;

    for (char c : text)
    {
        if (c == ';')
        {
            if (!current.empty())
            {
                out.push_back(current);
                current.clear();
            }
            continue;
        }

        current.push_back(c);
    }

    if (!current.empty())
    {
        out.push_back(current);
    }

    out.erase(std::remove_if(out.begin(), out.end(), [](const std::string& value) {
        return value.empty();
    }), out.end());

    return out;
}

bool parseU16(const std::string& text, uint16_t& value)
{
    try
    {
        const int parsed = std::stoi(text);
        if (parsed < 0 || parsed > 65535)
        {
            return false;
        }
        value = static_cast<uint16_t>(parsed);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool parseI64(const std::string& text, int64_t& value)
{
    try
    {
        value = std::stoll(text);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool parseBoolWord(const std::string& text, bool& value)
{
    if (text == "on" || text == "true" || text == "1")
    {
        value = true;
        return true;
    }

    if (text == "off" || text == "false" || text == "0")
    {
        value = false;
        return true;
    }

    return false;
}

std::string normalizeEmailForConsole(const std::string& value)
{
    if (value.find('@') != std::string::npos)
    {
        return value;
    }

    return value + "@localhost";
}

void printHelp()
{
    std::cout
        << "Commands:\n"
        << "  help\n"
        << "  quit | exit\n"
        << "  connect <smtpHost> <smtpPort> <imapHost> <imapPort> <user> <pass>\n"
        << "  disconnect\n"
        << "  send <from|from@domain> <to|to@domain> <subject> <body> [attachmentPath1;attachmentPath2;...]\n"
        << "  folders\n"
        << "  fetch <folder> [offset] [limit]\n"
        << "  fetchmail <seqId>\n"
        << "  getatt <seqId> <attachmentIndex> [outputPath]\n"
        << "  deletemail <seqId>\n"
        << "  read <seqId>\n"
        << "  move <seqId> <targetFolder>\n"
        << "  mkdir <folderName>\n"
        << "  rmdir <folderName>\n"
        << "  labeladd <seqId> <label>\n"
        << "  labelrm <seqId> <label>\n"
        << "  star <seqId> <on|off>\n"
        << "  important <seqId> <on|off>\n"
        << "\n"
        << "Tips:\n"
        << "  Use quotes for values with spaces.\n"
        << "  Example: send testmail2@localhost alice@localhost \"Test subject\" \"Hello from REPL\"\n"
        << "  Short aliases are auto-expanded to @localhost (alice -> alice@localhost).\n"
        << "  With files: send testmail2@localhost alice@localhost \"Report\" \"See attachment\" \"C:/tmp/a.txt;C:/tmp/b.pdf\"\n";
}

bool printResult(const MailResult& result)
{
    bool ok = true;

    std::visit([&](const auto& r)
    {
        using T = std::decay_t<decltype(r)>;

        if constexpr (std::is_same_v<T, ConnectResult>)
        {
            if (r.success) std::cout << "[OK] Connected\n";
            else
            {
                std::cout << "[FAIL] Connect: " << r.error << "\n";
                ok = false;
            }
        }
        else if constexpr (std::is_same_v<T, DisconnectResult>)
        {
            if (r.success) std::cout << "[OK] Disconnected\n";
            else
            {
                std::cout << "[FAIL] Disconnect: " << r.error << "\n";
                ok = false;
            }
        }
        else if constexpr (std::is_same_v<T, SendMailResult>)
        {
            if (r.success) std::cout << "[OK] Mail sent\n";
            else
            {
                std::cout << "[FAIL] Send: " << r.error << "\n";
                ok = false;
            }
        }
        else if constexpr (std::is_same_v<T, FetchFoldersResult>)
        {
            if (!r.success)
            {
                std::cout << "[FAIL] Folders: " << r.error << "\n";
                ok = false;
                return;
            }

            std::cout << "[OK] Folders count: " << r.folders.size() << "\n";
            for (const auto& folderName : r.folders)
            {
                std::cout << "  - " << folderName << "\n";
            }
        }
        else if constexpr (std::is_same_v<T, FetchMailsResult>)
        {
            if (!r.success)
            {
                std::cout << "[FAIL] Fetch mails: " << r.error << "\n";
                ok = false;
                return;
            }

            std::cout << "[OK] Mails fetched: " << r.mails.size() << "\n";
            for (std::size_t i = 0; i < r.mails.size(); ++i)
            {
                const auto& m = r.mails[i];
                std::cout << "  [" << (i + 1) << "]"
                          << " id=" << m.mailId
                          << " seen=" << (m.isSeen ? "Y" : "N")
                          << " flagged=" << (m.isFlagged ? "Y" : "N")
                          << " date=\"" << m.date << "\""
                          << " subject=\"" << m.subject << "\""
                          << " preview=\"" << m.preview << "\"\n";
            }
            std::cout << "  Note: mail IDs here are IMAP sequence numbers (1-based).\n";
        }
        else if constexpr (std::is_same_v<T, FetchMailResult>)
        {
            if (!r.success)
            {
                std::cout << "[FAIL] Fetch mail: " << r.error << "\n";
                ok = false;
                return;
            }

            std::cout << "[OK] Mail details:\n";
            std::cout << "  id=" << r.mail.mailId << "\n";
            std::cout << "  from=\"" << r.mail.sender << "\"\n";
            std::cout << "  subject=\"" << r.mail.subject << "\"\n";
            std::cout << "  date=\"" << r.mail.date << "\"\n";

            std::cout << "  to=";
            for (std::size_t i = 0; i < r.mail.to.size(); ++i)
            {
                if (i != 0) std::cout << ", ";
                std::cout << r.mail.to[i];
            }
            std::cout << "\n";

            std::cout << "  cc=";
            for (std::size_t i = 0; i < r.mail.cc.size(); ++i)
            {
                if (i != 0) std::cout << ", ";
                std::cout << r.mail.cc[i];
            }
            std::cout << "\n";

            std::cout << "  bcc=";
            for (std::size_t i = 0; i < r.mail.bcc.size(); ++i)
            {
                if (i != 0) std::cout << ", ";
                std::cout << r.mail.bcc[i];
            }
            std::cout << "\n";

            std::cout << "  reply-to=";
            for (std::size_t i = 0; i < r.mail.replyTo.size(); ++i)
            {
                if (i != 0) std::cout << ", ";
                std::cout << r.mail.replyTo[i];
            }
            std::cout << "\n";

            std::cout << "  attachments=" << r.mail.attachments.size() << "\n";
            for (std::size_t i = 0; i < r.mail.attachments.size(); ++i)
            {
                const auto& a = r.mail.attachments[i];
                std::cout << "    [" << (i + 1) << "] "
                          << a.fileName << " (" << a.sizeBytes << " bytes, " << a.mimeType << ")\n";
            }

            std::cout << "[OK] Mail body:\n" << r.mail.body << "\n";
        }
        else if constexpr (std::is_same_v<T, DownloadAttachmentResult>)
        {
            if (!r.success)
            {
                std::cout << "[FAIL] Attachment download: " << r.error << "\n";
                ok = false;
                return;
            }

            std::cout << "[OK] Attachment saved: " << r.outputPath
                      << " (" << r.bytesWritten << " bytes)\n";
        }
        else if constexpr (std::is_same_v<T, ReconnectStateResult>)
        {
            if (r.reconnecting)
            {
                std::cout << "[INFO] Reconnecting (attempt " << r.attempt << "): " << r.message << "\n";
            }
            else if (!r.message.empty())
            {
                std::cout << "[INFO] " << r.message << "\n";
            }
        }
        else if constexpr (std::is_same_v<T, MailActionResult>)
        {
            if (r.success) std::cout << "[OK] Action completed\n";
            else
            {
                std::cout << "[FAIL] Action: " << r.error << "\n";
                ok = false;
            }
        }
    }, result);

    return ok;
}

bool waitAndPrint(ResultInbox& inbox)
{
    while (true)
    {
        const auto result = inbox.waitPop(10000);
        if (!result.has_value())
        {
            std::cout << "[FAIL] Timeout waiting for server response\n";
            return false;
        }

        if (std::holds_alternative<ReconnectStateResult>(*result))
        {
            printResult(*result);
            continue;
        }

        return printResult(*result);
    }
}
} // namespace

int main()
{
    std::cout << "smtp_mua interactive mode\n";
    printHelp();

    CommandQueue queue;
    ResultInbox inbox;
    MailWorker worker(queue, [&](MailResult result) {
        inbox.push(std::move(result));
    });
    worker.start();

    bool hadError = false;
    bool isConnected = false;

    while (true)
    {
        std::cout << "mua> ";
        std::string line;
        if (!std::getline(std::cin, line))
        {
            break;
        }

        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        const auto args = splitArgs(line);
        if (args.empty())
        {
            continue;
        }

        const std::string& cmd = args[0];

        if (cmd == "help")
        {
            printHelp();
            continue;
        }

        if (cmd == "quit" || cmd == "exit")
        {
            break;
        }

        if (cmd == "connect")
        {
            if (args.size() != 7)
            {
                std::cout << "Usage: connect <smtpHost> <smtpPort> <imapHost> <imapPort> <user> <pass>\n";
                continue;
            }

            uint16_t smtpPort = 0;
            uint16_t imapPort = 0;
            if (!parseU16(args[2], smtpPort) || !parseU16(args[4], imapPort))
            {
                std::cout << "[FAIL] Invalid port\n";
                continue;
            }

            ConnectCommand c;
            c.SMTPhost = args[1];
            c.SMTPport = smtpPort;
            c.IMAPhost = args[3];
            c.IMAPport = imapPort;
            c.username = args[5];
            c.password = args[6];
            queue.push(c);
            if (!waitAndPrint(inbox)) hadError = true;
            else isConnected = true;
            continue;
        }

        if (cmd == "disconnect")
        {
            queue.push(DisconnectCommand{});
            if (!waitAndPrint(inbox)) hadError = true;
            else isConnected = false;
            continue;
        }

        if (cmd == "send")
        {
            if (args.size() < 5 || args.size() > 6)
            {
                std::cout << "Usage: send <from|from@domain> <to|to@domain> <subject> <body> [attachmentPath1;attachmentPath2;...]\n";
                continue;
            }

            SendMailCommand c;
            c.sender = normalizeEmailForConsole(args[1]);
            c.to = {normalizeEmailForConsole(args[2])};
            c.recipients = c.to;
            c.subject = args[3];
            c.body = args[4];
            if (args.size() == 6)
            {
                c.attachments = splitBySemicolon(args[5]);
            }
            queue.push(c);
            if (!waitAndPrint(inbox)) hadError = true;
            continue;
        }

        if (cmd == "folders")
        {
            queue.push(FetchFoldersCommand{});
            if (!waitAndPrint(inbox)) hadError = true;
            continue;
        }

        if (cmd == "fetch")
        {
            if (args.size() < 2 || args.size() > 4)
            {
                std::cout << "Usage: fetch <folder> [offset] [limit]\n";
                continue;
            }

            unsigned int offset = 0;
            unsigned int limit = 20;

            try
            {
                if (args.size() >= 3) offset = static_cast<unsigned int>(std::stoul(args[2]));
                if (args.size() >= 4) limit = static_cast<unsigned int>(std::stoul(args[3]));
            }
            catch (...)
            {
                std::cout << "[FAIL] Invalid offset/limit\n";
                continue;
            }

            queue.push(FetchMailsCommand{args[1], offset, limit});
            if (!waitAndPrint(inbox)) hadError = true;
            continue;
        }

        if (cmd == "fetchmail")
        {
            if (args.size() != 2)
            {
                std::cout << "Usage: fetchmail <seqId>\n";
                continue;
            }

            int64_t id = 0;
            if (!parseI64(args[1], id))
            {
                std::cout << "[FAIL] Invalid seqId\n";
                continue;
            }

            queue.push(FetchMailCommand{id});
            if (!waitAndPrint(inbox)) hadError = true;
            continue;
        }

        if (cmd == "getatt")
        {
            if (args.size() != 3 && args.size() != 4)
            {
                std::cout << "Usage: getatt <seqId> <attachmentIndex> [outputPath]\n";
                continue;
            }

            int64_t id = 0;
            if (!parseI64(args[1], id))
            {
                std::cout << "[FAIL] Invalid seqId\n";
                continue;
            }

            std::size_t attachmentIndex = 0;
            try
            {
                attachmentIndex = static_cast<std::size_t>(std::stoull(args[2]));
            }
            catch (...)
            {
                attachmentIndex = 0;
            }

            if (attachmentIndex == 0)
            {
                std::cout << "[FAIL] Invalid attachmentIndex\n";
                continue;
            }

            std::string outputPath;
            if (args.size() == 4)
            {
                outputPath = args[3];
            }

            queue.push(DownloadAttachmentCommand{id, attachmentIndex, outputPath});
            if (!waitAndPrint(inbox)) hadError = true;
            continue;
        }

        if (cmd == "deletemail")
        {
            if (args.size() != 2)
            {
                std::cout << "Usage: deletemail <seqId>\n";
                continue;
            }

            int64_t id = 0;
            if (!parseI64(args[1], id))
            {
                std::cout << "[FAIL] Invalid seqId\n";
                continue;
            }

            queue.push(DeleteMailCommand{id});
            if (!waitAndPrint(inbox)) hadError = true;
            continue;
        }

        if (cmd == "read")
        {
            if (args.size() != 2)
            {
                std::cout << "Usage: read <seqId>\n";
                continue;
            }

            int64_t id = 0;
            if (!parseI64(args[1], id))
            {
                std::cout << "[FAIL] Invalid seqId\n";
                continue;
            }

            queue.push(MarkMailReadCommand{id});
            if (!waitAndPrint(inbox)) hadError = true;
            continue;
        }

        if (cmd == "move")
        {
            if (args.size() != 3)
            {
                std::cout << "Usage: move <seqId> <targetFolder>\n";
                continue;
            }

            int64_t id = 0;
            if (!parseI64(args[1], id))
            {
                std::cout << "[FAIL] Invalid seqId\n";
                continue;
            }

            queue.push(MoveMailCommand{id, args[2]});
            if (!waitAndPrint(inbox)) hadError = true;
            continue;
        }

        if (cmd == "mkdir")
        {
            if (args.size() != 2)
            {
                std::cout << "Usage: mkdir <folderName>\n";
                continue;
            }

            queue.push(CreateFolderCommand{args[1]});
            if (!waitAndPrint(inbox)) hadError = true;
            continue;
        }

        if (cmd == "rmdir")
        {
            if (args.size() != 2)
            {
                std::cout << "Usage: rmdir <folderName>\n";
                continue;
            }

            queue.push(DeleteFolderCommand{args[1]});
            if (!waitAndPrint(inbox)) hadError = true;
            continue;
        }

        if (cmd == "labeladd")
        {
            if (args.size() != 3)
            {
                std::cout << "Usage: labeladd <seqId> <label>\n";
                continue;
            }

            int64_t id = 0;
            if (!parseI64(args[1], id))
            {
                std::cout << "[FAIL] Invalid seqId\n";
                continue;
            }

            queue.push(AddLabelCommand{id, args[2]});
            if (!waitAndPrint(inbox)) hadError = true;
            continue;
        }

        if (cmd == "labelrm")
        {
            if (args.size() != 3)
            {
                std::cout << "Usage: labelrm <seqId> <label>\n";
                continue;
            }

            int64_t id = 0;
            if (!parseI64(args[1], id))
            {
                std::cout << "[FAIL] Invalid seqId\n";
                continue;
            }

            queue.push(RemoveLabelCommand{id, args[2]});
            if (!waitAndPrint(inbox)) hadError = true;
            continue;
        }

        if (cmd == "star")
        {
            if (args.size() != 3)
            {
                std::cout << "Usage: star <seqId> <on|off>\n";
                continue;
            }

            int64_t id = 0;
            bool enabled = false;
            if (!parseI64(args[1], id) || !parseBoolWord(args[2], enabled))
            {
                std::cout << "[FAIL] Invalid args\n";
                continue;
            }

            queue.push(MarkMailStarredCommand{id, enabled});
            if (!waitAndPrint(inbox)) hadError = true;
            continue;
        }

        if (cmd == "important")
        {
            if (args.size() != 3)
            {
                std::cout << "Usage: important <seqId> <on|off>\n";
                continue;
            }

            int64_t id = 0;
            bool enabled = false;
            if (!parseI64(args[1], id) || !parseBoolWord(args[2], enabled))
            {
                std::cout << "[FAIL] Invalid args\n";
                continue;
            }

            queue.push(MarkMailImportantCommand{id, enabled});
            if (!waitAndPrint(inbox)) hadError = true;
            continue;
        }

        std::cout << "Unknown command: " << cmd << "\n";
        std::cout << "Type 'help' for command list.\n";
    }

    if (isConnected)
    {
        queue.push(DisconnectCommand{});
        waitAndPrint(inbox);
    }
    worker.stop();

    return hadError ? 1 : 0;
}
