#include "imap-session.hpp"

#include "ImapParser.hpp"
#include "ImapResponse.hpp"

IMAP_Session::IMAP_Session(boost::asio::ip::tcp::socket socket) : socket_(std::move(socket))
{
	handlers_ = {
		{ImapCommandType::LOGIN, [this](const ImapCommand& cmd) { return handleLogin(cmd); }},
		{ImapCommandType::LOGOUT, [this](const ImapCommand& cmd) { return handleLogout(cmd); }},
		{ImapCommandType::CAPABILITY, [this](const ImapCommand& cmd) { return handleCapability(cmd); }},
		{ImapCommandType::NOOP, [this](const ImapCommand& cmd) { return handleNoop(cmd); }},
		{ImapCommandType::SELECT, [this](const ImapCommand& cmd) { return handleSelect(cmd); }},
		{ImapCommandType::LIST, [this](const ImapCommand& cmd) { return handleList(cmd); }},
		{ImapCommandType::LSUB, [this](const ImapCommand& cmd) { return handleLsub(cmd); }},
		{ImapCommandType::STATUS, [this](const ImapCommand& cmd) { return handleStatus(cmd); }},
		{ImapCommandType::FETCH, [this](const ImapCommand& cmd) { return handleFetch(cmd); }},
		{ImapCommandType::STORE, [this](const ImapCommand& cmd) { return handleStore(cmd); }},
		{ImapCommandType::CREATE, [this](const ImapCommand& cmd) { return handleCreate(cmd); }},
		{ImapCommandType::DELETE, [this](const ImapCommand& cmd) { return handleDelete(cmd); }},
		{ImapCommandType::RENAME, [this](const ImapCommand& cmd) { return handleRename(cmd); }},
		{ImapCommandType::COPY, [this](const ImapCommand& cmd) { return handleCopy(cmd); }},
		{ImapCommandType::EXPUNGE, [this](const ImapCommand& cmd) { return handleExpunge(cmd); }},
	};
}

void IMAP_Session::start()
{
	sendBanner();
}

void IMAP_Session::sendBanner()
{
	writeResponse("* OK IMAP Server Ready\r\n");
	readCommand();
}

void IMAP_Session::readCommand()
{
	auto self = shared_from_this();
	boost::asio::async_read_until(socket_, buffer_, "\r\n",
								  [this, self](std::error_code ec, std::size_t)
								  {
									  if (!ec)
									  {
										  std::istream is(&buffer_);
										  std::string line;
										  std::getline(is, line);
										  if (!line.empty() && line.back() == '\r') line.pop_back();
										  handleCommand(line);
									  }
									  else
									  {
										  socket_.close();
									  }
								  });
}

bool IMAP_Session::requiresAuth(ImapCommandType type) const
{
	switch (type)
	{
	case ImapCommandType::LOGIN:
	case ImapCommandType::LOGOUT:
	case ImapCommandType::CAPABILITY:
	case ImapCommandType::NOOP:
		return false;
	default:
		return true;
	}
}

void IMAP_Session::handleCommand(const std::string& line)
{
	auto cmd = ImapParser::parse(line);

	if (cmd.type == ImapCommandType::UNKNOWN)
	{
		writeResponse(cmd.tag + " BAD Command not recognized\r\n");
		readCommand();
		return;
	}

	if (requiresAuth(cmd.type) && state_ == State::NON_AUTH)
	{
		writeResponse(cmd.tag + " BAD Not authenticated\r\n");
		readCommand();
		return;
	}

	auto it = handlers_.find(cmd.type);
	if (it != handlers_.end())
	{
		auto response = it->second(cmd);
		writeResponse(response);
	}
	else
	{
		writeResponse(cmd.tag + " BAD Command not implemented\r\n");
	}

	if (cmd.type != ImapCommandType::LOGOUT)
	{
		readCommand();
	}
}

void IMAP_Session::writeResponse(const std::string& msg)
{
	auto self = shared_from_this();
	boost::asio::async_write(socket_, boost::asio::buffer(msg), [this, self](std::error_code, std::size_t) {});
}

std::string IMAP_Session::handleLogin(const ImapCommand& cmd)
{
	if (cmd.args.size() < 2)
	{
		return cmd.tag + " BAD Missing arguments\r\n";
	}

	// TODO: Replace with Repo verification
	// if (repo_.verifyCredentials(cmd.args[0], cmd.args[1])) {
	//     authenticatedUser_ = cmd.args[0];
	//     state_ = State::AUTH;
	//     return cmd.tag + " OK Login successful\r\n";
	// }

	// Logger::warn("Failed login attempt for: " + cmd.args[0]);
	return cmd.tag + " NO Login failed\r\n";
}

std::string IMAP_Session::handleLogout(const ImapCommand& cmd)
{
	state_ = State::NON_AUTH;
	authenticatedUser_.clear();
	currentMailbox_ = {};
	return "* BYE Logout completed\r\n" + cmd.tag + " OK Logout completed\r\n";
}

std::string IMAP_Session::handleCapability(const ImapCommand& cmd)
{
	return ImapResponse::capability() + cmd.tag + " OK Capability completed\r\n";
}

std::string IMAP_Session::handleNoop(const ImapCommand& cmd)
{
	// TODO: Check for new messages in current mailbox from Repo
	// if (state_ == State::SELECTED) {
	//     auto updates = repo_.getMailboxUpdates(authenticatedUser_, currentMailbox_.name);
	//     // Return untagged EXISTS/RECENT if changed
	// }
	return cmd.tag + " OK Noop completed\r\n";
}

std::string IMAP_Session::handleSelect(const ImapCommand& cmd)
{
	if (cmd.args.empty())
	{
		return cmd.tag + " BAD Missing mailbox name\r\n";
	}

	// TODO: Replace with Repo call
	// auto mailbox = repo_.getMailbox(authenticatedUser_, cmd.args[0]);
	// if (!mailbox.exists) {
	//     return cmd.tag + " NO Mailbox not found\r\n";
	// }
	// currentMailbox_ = mailbox;
	// state_ = State::SELECTED;

	currentMailbox_.name = cmd.args[0];
	currentMailbox_.exists = 100;
	currentMailbox_.recent = 0;
	currentMailbox_.flags = {"\\Seen", "\\Answered", "\\Flagged", "\\Draft", "\\Recent"};
	currentMailbox_.readOnly = false;
	state_ = State::SELECTED;

	std::string response = ImapResponse::flags() + ImapResponse::exists(currentMailbox_.exists) +
						   ImapResponse::recent(currentMailbox_.recent);
	response += cmd.tag + " OK [READ-WRITE] Select completed\r\n";
	return response;
}

std::string IMAP_Session::handleList(const ImapCommand& cmd)
{
	// TODO: Replace with Repo call
	// auto mailboxes = repo_.listMailboxes(authenticatedUser_);

	std::string response = ImapResponse::untagged("LIST (\\HasNoChildren) \"/\" \"INBOX\"");
	response += cmd.tag + " OK List completed\r\n";
	return response;
}

std::string IMAP_Session::handleLsub(const ImapCommand& cmd)
{
	// TODO: Replace with Repo call
	// auto subscribed = repo_.listSubscribedMailboxes(authenticatedUser_);

	std::string response = ImapResponse::untagged("LSUB (\\HasNoChildren) \"/\" \"INBOX\"");
	response += cmd.tag + " OK Lsub completed\r\n";
	return response;
}

std::string IMAP_Session::handleStatus(const ImapCommand& cmd)
{
	if (cmd.args.empty())
	{
		return cmd.tag + " BAD Missing mailbox name\r\n";
	}

	// TODO: Replace with Repo call
	// auto status = repo_.getMailboxStatus(authenticatedUser_, cmd.args[0]);

	std::string response = ImapResponse::status(cmd.args[0], 100, 0, 5);
	response += cmd.tag + " OK Status completed\r\n";
	return response;
}

std::string IMAP_Session::handleFetch(const ImapCommand& cmd)
{
	if (state_ != State::SELECTED)
	{
		return cmd.tag + " BAD No mailbox selected\r\n";
	}

	if (cmd.args.size() < 2)
	{
		return cmd.tag + " BAD Missing arguments\r\n";
	}

	// TODO: Replace with Repo call
	// auto messages = repo_.getMessages(authenticatedUser_, currentMailbox_.name, cmd.args[0], cmd.args[1]);

	std::string response = ImapResponse::fetch(1, "(FLAGS (\\Seen) RFC822.TEXT {4}\r\ntest)");
	response += cmd.tag + " OK Fetch completed\r\n";
	return response;
}

std::string IMAP_Session::handleStore(const ImapCommand& cmd)
{
	if (state_ != State::SELECTED)
	{
		return cmd.tag + " BAD No mailbox selected\r\n";
	}

	if (cmd.args.size() < 3)
	{
		return cmd.tag + " BAD Missing arguments\r\n";
	}

	// TODO: Replace with Repo call
	// repo_.updateMessageFlags(authenticatedUser_, currentMailbox_.name, cmd.args[0], cmd.args[2]);

	return cmd.tag + " OK Store completed\r\n";
}

std::string IMAP_Session::handleCreate(const ImapCommand& cmd)
{
	if (cmd.args.empty())
	{
		return cmd.tag + " BAD Missing mailbox name\r\n";
	}

	// TODO: Replace with Repo call
	// if (!repo_.createMailbox(authenticatedUser_, cmd.args[0])) {
	//     return cmd.tag + " NO Create failed\r\n";
	// }

	return cmd.tag + " OK Create completed\r\n";
}

std::string IMAP_Session::handleDelete(const ImapCommand& cmd)
{
	if (cmd.args.empty())
	{
		return cmd.tag + " BAD Missing mailbox name\r\n";
	}

	// TODO: Replace with Repo call
	// if (!repo_.deleteMailbox(authenticatedUser_, cmd.args[0])) {
	//     return cmd.tag + " NO Delete failed\r\n";
	// }

	return cmd.tag + " OK Delete completed\r\n";
}

std::string IMAP_Session::handleRename(const ImapCommand& cmd)
{
	if (cmd.args.size() < 2)
	{
		return cmd.tag + " BAD Missing arguments\r\n";
	}

	// TODO: Replace with Repo call
	// if (!repo_.renameMailbox(authenticatedUser_, cmd.args[0], cmd.args[1])) {
	//     return cmd.tag + " NO Rename failed\r\n";
	// }

	return cmd.tag + " OK Rename completed\r\n";
}

std::string IMAP_Session::handleCopy(const ImapCommand& cmd)
{
	if (state_ != State::SELECTED)
	{
		return cmd.tag + " BAD No mailbox selected\r\n";
	}

	if (cmd.args.size() < 2)
	{
		return cmd.tag + " BAD Missing arguments\r\n";
	}

	// TODO: Replace with Repo call
	// if (!repo_.copyMessages(authenticatedUser_, currentMailbox_.name, cmd.args[0], cmd.args[1])) {
	//     return cmd.tag + " NO Copy failed\r\n";
	// }

	return cmd.tag + " OK Copy completed\r\n";
}

std::string IMAP_Session::handleExpunge(const ImapCommand& cmd)
{
	if (state_ != State::SELECTED)
	{
		return cmd.tag + " BAD No mailbox selected\r\n";
	}

	// TODO: Replace with Repo call
	// auto deleted = repo_.expungeDeleted(authenticatedUser_, currentMailbox_.name);
	// for (auto msgNum : deleted) {
	//     response += ImapResponse::untagged(std::to_string(msgNum) + " EXPUNGE");
	// }

	return cmd.tag + " OK Expunge completed\r\n";
}
