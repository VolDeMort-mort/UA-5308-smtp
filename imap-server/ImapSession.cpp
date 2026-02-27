#include "ImapSession.hpp"

#include <sstream>

#include "ImapParser.hpp"
#include "ImapResponse.hpp"
#include "ImapUtils.hpp"

using namespace ImapResponse;

ImapSession::ImapSession(boost::asio::ip::tcp::socket socket, Logger& logger)
	: m_socket(std::move(socket)), m_logger(logger)
{
	m_logger.Log(PROD, "New ImapSession created");
	m_logger.Log(TRACE, "ImapSession::ImapSession - socket accepted");
	m_handlers = {
		{ImapCommandType::Login, [this](const ImapCommand& cmd) { return HandleLogin(cmd); }},
		{ImapCommandType::Logout, [this](const ImapCommand& cmd) { return HandleLogout(cmd); }},
		{ImapCommandType::Capability, [this](const ImapCommand& cmd) { return HandleCapability(cmd); }},
		{ImapCommandType::Noop, [this](const ImapCommand& cmd) { return HandleNoop(cmd); }},
		{ImapCommandType::Select, [this](const ImapCommand& cmd) { return HandleSelect(cmd); }},
		{ImapCommandType::List, [this](const ImapCommand& cmd) { return HandleList(cmd); }},
		{ImapCommandType::Lsub, [this](const ImapCommand& cmd) { return HandleLsub(cmd); }},
		{ImapCommandType::Status, [this](const ImapCommand& cmd) { return HandleStatus(cmd); }},
		{ImapCommandType::Fetch, [this](const ImapCommand& cmd) { return HandleFetch(cmd); }},
		{ImapCommandType::Store, [this](const ImapCommand& cmd) { return HandleStore(cmd); }},
		{ImapCommandType::Create, [this](const ImapCommand& cmd) { return HandleCreate(cmd); }},
		{ImapCommandType::Delete, [this](const ImapCommand& cmd) { return HandleDelete(cmd); }},
		{ImapCommandType::Rename, [this](const ImapCommand& cmd) { return HandleRename(cmd); }},
		{ImapCommandType::Copy, [this](const ImapCommand& cmd) { return HandleCopy(cmd); }},
		{ImapCommandType::Expunge, [this](const ImapCommand& cmd) { return HandleExpunge(cmd); }},
	};
}

void ImapSession::Start()
{
	m_logger.Log(DEBUG, "ImapSession::Start - Start");
	SendBanner();
	m_logger.Log(DEBUG, "ImapSession::Start - End");
}

void ImapSession::SendBanner()
{
	m_logger.Log(DEBUG, "ImapSession::SendBanner - Start");
	WriteResponse("* OK IMAP Server Ready\r\n");
	m_logger.Log(PROD, "ImapSession::SendBanner - Banner sent");
	m_logger.Log(DEBUG, "ImapSession::SendBanner - End");
	ReadCommand();
}

void ImapSession::ReadCommand()
{
	m_logger.Log(DEBUG, "ImapSession::ReadCommand - Start");
	auto self = shared_from_this();
	boost::asio::async_read_until(m_socket, m_buffer, "\r\n",
								  [this, self](std::error_code ec, std::size_t)
								  {
									  if (!ec)
									  {
										  std::istream is(&m_buffer);
										  std::string line;
										  std::getline(is, line);
										  if (!line.empty() && line.back() == '\r')
										  {
											  line.pop_back();
										  }
										  m_logger.Log(DEBUG, "ImapSession::ReadCommand - Acquired: " + line);
										  m_logger.Log(TRACE, "ImapSession::ReadCommand - Calling HandleCommand");
										  HandleCommand(line);
									  }
									  else
									  {
										  m_logger.Log(PROD, "ImapSession::ReadCommand - Error: " + ec.message());
										  m_socket.close();
									  }
									  m_logger.Log(DEBUG, "ImapSession::ReadCommand - End");
								  });
}

bool ImapSession::RequiresAuth(ImapCommandType type) const
{
	m_logger.Log(TRACE, "ImapSession::RequiresAuth - In: type=" + IMAP_UTILS::CommandTypeToString(type));
	m_logger.Log(DEBUG, "ImapSession::RequiresAuth - Start");

	bool result;
	switch (type)
	{
	case ImapCommandType::Login:
	case ImapCommandType::Logout:
	case ImapCommandType::Capability:
	case ImapCommandType::Noop:
		result = false;
		break;
	default:
		result = true;
		break;
	}

	m_logger.Log(TRACE, "ImapSession::RequiresAuth - Out: " + std::string(result ? "true" : "false"));
	m_logger.Log(DEBUG, "ImapSession::RequiresAuth - End");
	return result;
}

void ImapSession::HandleCommand(const std::string& line)
{
	m_logger.Log(TRACE, "ImapSession::HandleCommand - In: line=" + line);
	m_logger.Log(DEBUG, "ImapSession::HandleCommand - Start");

	auto cmd = ImapParser::Parse(line);

	if (cmd.m_type == ImapCommandType::Unknown)
	{
		m_logger.Log(PROD, "ImapSession::HandleCommand - Unknown command");
		std::string response = cmd.m_tag + " BAD Command not recognized\r\n";
		WriteResponse(response);
		m_logger.Log(TRACE, "ImapSession::HandleCommand - Out: " + response);
		m_logger.Log(DEBUG, "ImapSession::HandleCommand - End");
		ReadCommand();
		return;
	}

	if (RequiresAuth(cmd.m_type) && m_state == SessionState::NonAuthenticated)
	{
		m_logger.Log(PROD, "ImapSession::HandleCommand - User not authenticated");
		std::string response = cmd.m_tag + " BAD Not authenticated\r\n";
		WriteResponse(response);
		m_logger.Log(TRACE, "ImapSession::HandleCommand - Out: " + response);
		m_logger.Log(DEBUG, "ImapSession::HandleCommand - End");
		ReadCommand();
		return;
	}

	auto it = m_handlers.find(cmd.m_type);
	if (it != m_handlers.end())
	{
		auto response = it->second(cmd);
		WriteResponse(response);
		m_logger.Log(TRACE, "ImapSession::HandleCommand - Out: " + response);
	}
	else
	{
		m_logger.Log(PROD, "ImapSession::HandleCommand - Command not implemented");
		std::string response = cmd.m_tag + " BAD Command not implemented\r\n";
		WriteResponse(response);
		m_logger.Log(TRACE, "ImapSession::HandleCommand - Out: " + response);
	}

	m_logger.Log(DEBUG, "ImapSession::HandleCommand - End");

	if (cmd.m_type != ImapCommandType::Logout)
	{
		ReadCommand();
	}
}

void ImapSession::WriteResponse(const std::string& msg)
{
	m_logger.Log(TRACE, "ImapSession::WriteResponse - In: msg length=" + std::to_string(msg.size()));
	m_logger.Log(DEBUG, "ImapSession::WriteResponse - Start");

	auto self = shared_from_this();
	boost::asio::async_write(m_socket, boost::asio::buffer(msg),
							 [this, self](std::error_code ec, std::size_t bytes_transferred)
							 {
								 if (ec)
								 {
									 m_logger.Log(PROD, "ImapSession::WriteResponse - Error: " + ec.message());
								 }
								 else
								 {
									 m_logger.Log(DEBUG, "ImapSession::WriteResponse - Sent " +
															 std::to_string(bytes_transferred) + " bytes");
								 }
								 m_logger.Log(DEBUG, "ImapSession::WriteResponse - End");
							 });
}

// Command

std::string ImapSession::HandleLogin(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleLogin - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleLogin - Start");

	if (cmd.m_args.size() < 2)
	{
		std::string response = cmd.m_tag + " BAD Missing arguments\r\n";
		m_logger.Log(TRACE, "ImapSession::HandleLogin - Out: " + response);
		m_logger.Log(DEBUG, "ImapSession::HandleLogin - End");
		return response;
	}

	std::string response = cmd.m_tag + " NO Login failed\r\n";
	m_logger.Log(PROD, "ImapSession::HandleLogin - Login failed, no auth implemented");
	m_logger.Log(TRACE, "ImapSession::HandleLogin - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleLogin - End");
	return response;
}

std::string ImapSession::HandleLogout(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleLogout - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleLogout - Start");

	m_state = SessionState::NonAuthenticated;
	m_authenticatedUser.clear();
	m_currentMailbox = {};

	std::string response = "* BYE Logout completed\r\n" + cmd.m_tag + " OK Logout completed\r\n";
	m_logger.Log(PROD, "ImapSession::HandleLogout - User logged out");
	m_logger.Log(TRACE, "ImapSession::HandleLogout - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleLogout - End");
	return response;
}

std::string ImapSession::HandleCapability(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleCapability - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleCapability - Start");

	std::string response = ImapResponse::Capability() + cmd.m_tag + " OK Capability completed\r\n";

	m_logger.Log(TRACE, "ImapSession::HandleCapability - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleCapability - End");
	return response;
}

std::string ImapSession::HandleNoop(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleNoop - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleNoop - Start");

	std::string response = cmd.m_tag + " OK Noop completed\r\n";

	m_logger.Log(TRACE, "ImapSession::HandleNoop - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleNoop - End");
	return response;
}

std::string ImapSession::HandleSelect(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleSelect - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleSelect - Start");

	if (cmd.m_args.empty())
	{
		std::string response = cmd.m_tag + " BAD Missing mailbox name\r\n";
		m_logger.Log(TRACE, "ImapSession::HandleSelect - Out: " + response);
		m_logger.Log(DEBUG, "ImapSession::HandleSelect - End");
		return response;
	}

	m_currentMailbox.m_name = cmd.m_args[0];
	m_currentMailbox.m_exists = 100;
	m_currentMailbox.m_recent = 0;
	m_currentMailbox.m_flags = {"\\Seen", "\\Answered", "\\Flagged", "\\Draft", "\\Recent"};
	m_currentMailbox.m_readOnly = false;
	m_state = SessionState::Selected;

	std::string response = ImapResponse::Flags() + ImapResponse::Exists(m_currentMailbox.m_exists) +
						   ImapResponse::Recent(m_currentMailbox.m_recent);
	response += cmd.m_tag + " OK [READ-WRITE] Select completed\r\n";

	m_logger.Log(PROD, "ImapSession::HandleSelect - Mailbox selected: " + m_currentMailbox.m_name);
	m_logger.Log(TRACE, "ImapSession::HandleSelect - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleSelect - End");
	return response;
}

std::string ImapSession::HandleList(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleList - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleList - Start");

	std::string response = ImapResponse::Untagged("LIST (\\HasNoChildren) \"/\" \"INBOX\"");
	response += cmd.m_tag + " OK List completed\r\n";

	m_logger.Log(TRACE, "ImapSession::HandleList - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleList - End");
	return response;
}

std::string ImapSession::HandleLsub(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleLsub - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleLsub - Start");

	std::string response = ImapResponse::Untagged("LSUB (\\HasNoChildren) \"/\" \"INBOX\"");
	response += cmd.m_tag + " OK Lsub completed\r\n";

	m_logger.Log(TRACE, "ImapSession::HandleLsub - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleLsub - End");
	return response;
}

std::string ImapSession::HandleStatus(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleStatus - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleStatus - Start");

	if (cmd.m_args.empty())
	{
		std::string response = cmd.m_tag + " BAD Missing mailbox name\r\n";
		m_logger.Log(TRACE, "ImapSession::HandleStatus - Out: " + response);
		m_logger.Log(DEBUG, "ImapSession::HandleStatus - End");
		return response;
	}

	std::string response = ImapResponse::Status(cmd.m_args[0], 100, 0, 5);
	response += cmd.m_tag + " OK Status completed\r\n";

	m_logger.Log(TRACE, "ImapSession::HandleStatus - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleStatus - End");
	return response;
}

std::string ImapSession::HandleFetch(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleFetch - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleFetch - Start");

	if (m_state != SessionState::Selected)
	{
		std::string response = cmd.m_tag + " BAD No mailbox selected\r\n";
		m_logger.Log(TRACE, "ImapSession::HandleFetch - Out: " + response);
		m_logger.Log(DEBUG, "ImapSession::HandleFetch - End");
		return response;
	}

	if (cmd.m_args.size() < 2)
	{
		std::string response = cmd.m_tag + " BAD Missing arguments\r\n";
		m_logger.Log(TRACE, "ImapSession::HandleFetch - Out: " + response);
		m_logger.Log(DEBUG, "ImapSession::HandleFetch - End");
		return response;
	}

	std::string response = ImapResponse::Fetch(1, "(FLAGS (\\Seen) RFC822.TEXT {4}\r\ntest)");
	response += cmd.m_tag + " OK Fetch completed\r\n";

	m_logger.Log(TRACE, "ImapSession::HandleFetch - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleFetch - End");
	return response;
}

std::string ImapSession::HandleStore(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleStore - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleStore - Start");

	if (m_state != SessionState::Selected)
	{
		std::string response = cmd.m_tag + " BAD No mailbox selected\r\n";
		m_logger.Log(TRACE, "ImapSession::HandleStore - Out: " + response);
		m_logger.Log(DEBUG, "ImapSession::HandleStore - End");
		return response;
	}

	if (cmd.m_args.size() < 3)
	{
		std::string response = cmd.m_tag + " BAD Missing arguments\r\n";
		m_logger.Log(TRACE, "ImapSession::HandleStore - Out: " + response);
		m_logger.Log(DEBUG, "ImapSession::HandleStore - End");
		return response;
	}

	std::string response = cmd.m_tag + " OK Store completed\r\n";

	m_logger.Log(TRACE, "ImapSession::HandleStore - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleStore - End");
	return response;
}

std::string ImapSession::HandleCreate(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleCreate - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleCreate - Start");

	if (cmd.m_args.empty())
	{
		std::string response = cmd.m_tag + " BAD Missing mailbox name\r\n";
		m_logger.Log(TRACE, "ImapSession::HandleCreate - Out: " + response);
		m_logger.Log(DEBUG, "ImapSession::HandleCreate - End");
		return response;
	}

	std::string response = cmd.m_tag + " OK Create completed\r\n";

	m_logger.Log(TRACE, "ImapSession::HandleCreate - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleCreate - End");
	return response;
}

std::string ImapSession::HandleDelete(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleDelete - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleDelete - Start");

	if (cmd.m_args.empty())
	{
		std::string response = cmd.m_tag + " BAD Missing mailbox name\r\n";
		m_logger.Log(TRACE, "ImapSession::HandleDelete - Out: " + response);
		m_logger.Log(DEBUG, "ImapSession::HandleDelete - End");
		return response;
	}

	std::string response = cmd.m_tag + " OK Delete completed\r\n";

	m_logger.Log(TRACE, "ImapSession::HandleDelete - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleDelete - End");
	return response;
}

std::string ImapSession::HandleRename(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleRename - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleRename - Start");

	if (cmd.m_args.size() < 2)
	{
		std::string response = cmd.m_tag + " BAD Missing arguments\r\n";
		m_logger.Log(TRACE, "ImapSession::HandleRename - Out: " + response);
		m_logger.Log(DEBUG, "ImapSession::HandleRename - End");
		return response;
	}

	std::string response = cmd.m_tag + " OK Rename completed\r\n";

	m_logger.Log(TRACE, "ImapSession::HandleRename - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleRename - End");
	return response;
}

std::string ImapSession::HandleCopy(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleCopy - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleCopy - Start");

	if (m_state != SessionState::Selected)
	{
		std::string response = cmd.m_tag + " BAD No mailbox selected\r\n";
		m_logger.Log(TRACE, "ImapSession::HandleCopy - Out: " + response);
		m_logger.Log(DEBUG, "ImapSession::HandleCopy - End");
		return response;
	}

	if (cmd.m_args.size() < 2)
	{
		std::string response = cmd.m_tag + " BAD Missing arguments\r\n";
		m_logger.Log(TRACE, "ImapSession::HandleCopy - Out: " + response);
		m_logger.Log(DEBUG, "ImapSession::HandleCopy - End");
		return response;
	}

	std::string response = cmd.m_tag + " OK Copy completed\r\n";

	m_logger.Log(TRACE, "ImapSession::HandleCopy - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleCopy - End");
	return response;
}

std::string ImapSession::HandleExpunge(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleExpunge - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleExpunge - Start");

	if (m_state != SessionState::Selected)
	{
		std::string response = cmd.m_tag + " BAD No mailbox selected\r\n";
		m_logger.Log(TRACE, "ImapSession::HandleExpunge - Out: " + response);
		m_logger.Log(DEBUG, "ImapSession::HandleExpunge - End");
		return response;
	}

	std::string response = cmd.m_tag + " OK Expunge completed\r\n";

	m_logger.Log(TRACE, "ImapSession::HandleExpunge - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleExpunge - End");
	return response;
}
