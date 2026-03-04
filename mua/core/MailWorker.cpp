#include "MailWorker.hpp"

#include "SmtpSession.hpp"
#include "SocketConnector.hpp"
#include "SocketConnection.hpp"

// IMAP port is taken from ConnectCommand.IMAPport (typically 2553)

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
	m_thread = std::thread(&MailWorker::run, this);
}

void MailWorker::stop()
{
	m_queue.stop();
	if (m_thread.joinable())
		m_thread.join();
}

// -------------------------------------------------------------------
// IMAP helpers
// -------------------------------------------------------------------

std::string MailWorker::nextTag()
{
	return "A" + std::to_string(++m_tagCounter);
}

// Send an IMAP command and read all lines until the tagged response.
// Returns { success, list of untagged "* ..." lines }
std::pair<bool, std::vector<std::string>> MailWorker::imapExchange(const std::string& cmd)
{
	std::string tag = nextTag();

	// send: "A1 LIST "" *\r\n"
	if (!m_imapConnection->send(tag + " " + cmd + "\r\n"))
		return {false, {}};

	std::vector<std::string> untagged;

	// read lines until our tag appears
	while (true)
	{
		std::string line;
		if (!m_imapConnection->receive(line))
			return {false, untagged};

		// final tagged response
		if (line.rfind(tag + " OK", 0) == 0) return {true, untagged};
		if (line.rfind(tag + " NO", 0) == 0) return {false, untagged};
		if (line.rfind(tag + " BAD", 0) == 0) return {false, untagged};

		// accumulate untagged data
		untagged.push_back(line);
	}
}

// -------------------------------------------------------------------
// Main loop
// -------------------------------------------------------------------

void MailWorker::run()
{
	while (auto cmd = m_queue.pop())
	{
		std::visit([this](const auto& c) {
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
		}, *cmd);
	}
}

// -------------------------------------------------------------------
// SMTP handlers
// -------------------------------------------------------------------

void MailWorker::handleConnect(const ConnectCommand& cmd)
{
	m_connector = std::make_unique<SocketConnector>();
	m_connector->initialize(m_ioContext);

	m_smtpHost = cmd.SMTPhost;
	m_smtpPort = cmd.SMTPport;

	// --- connect SMTP --- (probe only; reconnect per message)
	if (!m_connector->connect(cmd.SMTPhost, cmd.SMTPport, m_smtpConnection))
	{
		m_onResult(ConnectResult{false, "Failed to connect to SMTP server"});
		return;
	}
	m_smtpSession = std::make_unique<SmtpSession>(cmd.SMTPhost);
	std::string greeting;
	m_smtpConnection->receive(greeting); // read "220 ..."
	// close immediately — will reconnect per message
	m_smtpConnection->close();
	m_smtpConnection.reset();

	// --- connect IMAP ---
	if (!m_connector->connect(cmd.IMAPhost, cmd.IMAPport, m_imapConnection))
	{
		m_onResult(ConnectResult{false, "Failed to connect to IMAP server"});
		return;
	}

	std::string banner;
	m_imapConnection->receive(banner); // read "* OK IMAP Server Ready"

	// --- IMAP login ---
	auto [loginOk, loginLines] = imapExchange("LOGIN " + cmd.username + " " + cmd.password);
	if (!loginOk)
	{
		// server returned NO — authentication rejected
		m_imapConnection->close();
		m_imapConnection.reset();
		m_onResult(ConnectResult{false, "IMAP login failed (server returned NO)"});
		return;
	}

	m_tagCounter = 0; // reset tag counter after login
	m_onResult(ConnectResult{true, {}});
}

void MailWorker::handleDisconnect(const DisconnectCommand&)
{
	// close IMAP
	if (m_imapConnection)
	{
		imapExchange("LOGOUT"); // "* BYE..." + "A1 OK"
		m_imapConnection->close();
		m_imapConnection.reset();
	}

	// close SMTP
	if (m_smtpConnection)
	{
		m_smtpConnection->close();
		m_smtpConnection.reset();
	}

	m_smtpSession.reset();
	m_connector.reset();
	m_currentFolder.clear();
	m_tagCounter = 0;

	m_onResult(DisconnectResult{true, {}});
}

void MailWorker::handleSendMail(const SendMailCommand& cmd)
{
	// SMTP — reconnect for each message.
	// The server closes the connection after QUIT, so keep-alive is not used.
	if (m_smtpHost.empty())
	{
		m_onResult(SendMailResult{false, "Not connected — call connect first"});
		return;
	}

	std::unique_ptr<SocketConnection> smtpConn;
	if (!m_connector->connect(m_smtpHost, m_smtpPort, smtpConn))
	{
		m_onResult(SendMailResult{false, "Failed to reconnect to SMTP"});
		return;
	}

	// read "220 ..."
	std::string greeting;
	smtpConn->receive(greeting);

	auto sendLine = [&](const std::string& line) -> bool {
		std::string response;
		if (!smtpConn->send(line + "\r\n")) return false;
		if (!smtpConn->receive(response)) return false;
		return true;
	};

	if (!sendLine("HELO " + cmd.sender))             { m_onResult(SendMailResult{false, "HELO failed"});      return; }
	if (!sendLine("MAIL FROM:<" + cmd.sender + ">")) { m_onResult(SendMailResult{false, "MAIL FROM failed"}); return; }

	for (const auto& rcpt : cmd.recipients)
		if (!sendLine("RCPT TO:<" + rcpt + ">"))     { m_onResult(SendMailResult{false, "RCPT TO failed"});   return; }

	if (!sendLine("DATA"))                           { m_onResult(SendMailResult{false, "DATA failed"});      return; }

	std::string body = "Subject: " + cmd.subject + "\r\n\r\n" + cmd.body + "\r\n.\r\n";
	std::string response;
	smtpConn->send(body);
	smtpConn->receive(response);
	smtpConn->send("QUIT\r\n");
	smtpConn->receive(response);
	smtpConn->close();

	m_onResult(SendMailResult{true, {}});
}

// -------------------------------------------------------------------
// IMAP handlers
// -------------------------------------------------------------------

void MailWorker::handleFetchFolders(const FetchFoldersCommand&)
{
	if (!m_imapConnection)
	{
		m_onResult(FetchFoldersResult{false, "Not connected", {}});
		return;
	}

	// LIST "" * — request all folders
	// Server replies: * LIST (\HasNoChildren) "/" "INBOX"
	//                 A2 OK List completed
	auto [ok, lines] = imapExchange("LIST \"\" \"*\"");
	if (!ok)
	{
		m_onResult(FetchFoldersResult{false, "LIST failed", {}});
		return;
	}

	std::vector<Folder> folders;
	for (const auto& line : lines)
	{
		// parse lines like: * LIST (...) "/" "FolderName"
		auto pos = line.rfind('"');
		if (pos == std::string::npos) continue;
		auto start = line.rfind('"', pos - 1);
		if (start == std::string::npos) continue;

		Folder f;
		f.name = line.substr(start + 1, pos - start - 1);
		folders.push_back(f);
	}

	m_onResult(FetchFoldersResult{true, {}, folders});
}

void MailWorker::handleFetchMails(const FetchMailsCommand& cmd)
{
	if (!m_imapConnection)
	{
		m_onResult(FetchMailsResult{false, "Not connected", {}});
		return;
	}

	// SELECT the folder if it changed
	if (m_currentFolder != cmd.folderName)
	{
		// SELECT INBOX
		// Server replies: * FLAGS (...), * N EXISTS, * N RECENT, A3 OK [READ-WRITE] ...
		auto [ok, _] = imapExchange("SELECT \"" + cmd.folderName + "\"");
		if (!ok)
		{
			m_onResult(FetchMailsResult{false, "SELECT failed", {}});
			return;
		}
		m_currentFolder = cmd.folderName;
	}

	// FETCH 1:limit — request message headers
	// Server replies: * 1 FETCH (FLAGS (\Seen) RFC822.TEXT {N}\r\n...), A4 OK
	unsigned int from = cmd.offset + 1;
	unsigned int to   = cmd.offset + cmd.limit;
	auto [ok, lines] = imapExchange(
	    "FETCH " + std::to_string(from) + ":" + std::to_string(to) +
	    " (FLAGS SUBJECT FROM DATE)");

	if (!ok)
	{
		m_onResult(FetchMailsResult{false, "FETCH failed", {}});
		return;
	}

	std::vector<Message> mails;
	for (const auto& line : lines)
	{
		// lines like: * N FETCH (FLAGS (\Seen) RFC822.TEXT {4}\r\ntest)
		if (line.rfind("* ", 0) != 0) continue;

		Message msg;
		msg.is_seen      = line.find("\\Seen")      != std::string::npos;
		msg.is_starred   = line.find("\\Flagged")   != std::string::npos;
		msg.is_important = line.find("$Important") != std::string::npos; // non-standard flag
		msg.status = MessageStatus::Received;

		// extract SUBJECT if present
		auto subPos = line.find("SUBJECT \"");
		if (subPos != std::string::npos)
		{
			subPos += 9;
			auto end = line.find('"', subPos);
			if (end != std::string::npos)
				msg.subject = line.substr(subPos, end - subPos);
		}

		mails.push_back(msg);
	}

	m_onResult(FetchMailsResult{true, {}, mails});
}

void MailWorker::handleFetchMail(const FetchMailCommand& cmd)
{
	if (!m_imapConnection)
	{
		m_onResult(FetchMailResult{false, "Not connected", {}});
		return;
	}

	// FETCH mailId (RFC822) — full message
	// Server: * 1 FETCH (FLAGS (\Seen) RFC822.TEXT {N}\r\nbody)
	auto [ok, lines] = imapExchange(
	    "FETCH " + std::to_string(cmd.mailId) + " (RFC822)");

	if (!ok || lines.empty())
	{
		m_onResult(FetchMailResult{false, "FETCH failed", {}});
		return;
	}

	Message msg;
	msg.status = MessageStatus::Received;

	// collect body — everything after the first FETCH line
	std::string body;
	bool inBody = false;
	for (const auto& line : lines)
	{
		if (!inBody && line.find("FETCH") != std::string::npos)
		{
			inBody = true;
			continue;
		}
		if (inBody) body += line + "\n";
	}
	msg.body = body;

	m_onResult(FetchMailResult{true, {}, msg});
}

void MailWorker::handleDeleteMail(const DeleteMailCommand& cmd)
{
	if (!m_imapConnection)
	{
		m_onResult(MailActionResult{false, "Not connected"});
		return;
	}

	// STORE mailId +FLAGS (\Deleted) — mark for deletion
	auto [storeOk, _1] = imapExchange(
	    "STORE " + std::to_string(cmd.mailId) + " +FLAGS (\\Deleted)");
	if (!storeOk)
	{
		m_onResult(MailActionResult{false, "STORE failed"});
		return;
	}

	// EXPUNGE — physically remove flagged messages
	auto [expOk, _2] = imapExchange("EXPUNGE");
	m_onResult(MailActionResult{expOk, expOk ? "" : "EXPUNGE failed"});
}

void MailWorker::handleMarkMailRead(const MarkMailReadCommand& cmd)
{
	if (!m_imapConnection)
	{
		m_onResult(MailActionResult{false, "Not connected"});
		return;
	}

	// STORE mailId +FLAGS (\Seen)
	auto [ok, _] = imapExchange(
	m_onResult(MailActionResult{ok, ok ? "" : "DELETE failed"});
}

void MailWorker::handleAddLabel(const AddLabelCommand& cmd)
{
	if (!m_imapConnection)
	{
		m_onResult(MailActionResult{false, "Not connected"});
		return;
	}

	// STORE mailId +FLAGS (label)
	auto [ok, _] = imapExchange(
	    "STORE " + std::to_string(cmd.mailId) + " -FLAGS (" + cmd.label + ")");
	m_onResult(MailActionResult{ok, ok ? "" : "STORE failed"});
}

void MailWorker::handleMarkMailStarred(const MarkMailStarredCommand& cmd)
{
	if (!m_imapConnection)
	{
		m_onResult(MailActionResult{false, "Not connected"});
		return;
	}

	// \Flagged is the standard IMAP flag for "starred"
	std::string flags = cmd.starred ? "+FLAGS (\\Flagged)" : "-FLAGS (\\Flagged)";
	auto [ok, _] = imapExchange("STORE " + std::to_string(cmd.mailId) + " " + flags);
	m_onResult(MailActionResult{ok, ok ? "" : "STORE failed"});
}

void MailWorker::handleMarkMailImportant(const MarkMailImportantCommand& cmd)
{
	if (!m_imapConnection)
	{
		m_onResult(MailActionResult{false, "Not connected"});
		return;
	}

	// non-standard $Important flag (Gmail-style)
	std::string flags = cmd.important ? "+FLAGS ($Important)" : "-FLAGS ($Important)";
	auto [ok, _] = imapExchange("STORE " + std::to_string(cmd.mailId) + " " + flags);
	m_onResult(MailActionResult{ok, ok ? "" : "STORE failed"});
}

