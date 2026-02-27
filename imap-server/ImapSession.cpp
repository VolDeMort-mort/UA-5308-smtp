#include "ImapSession.hpp"

#include "ImapParser.hpp"
#include "ImapResponse.hpp"

using namespace ImapResponse;

ImapSession::ImapSession(boost::asio::ip::tcp::socket socket) : m_socket(std::move(socket))
{
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
	SendBanner();
}

void ImapSession::SendBanner()
{
	WriteResponse("* OK IMAP Server Ready\r\n");
	ReadCommand();
}

void ImapSession::ReadCommand()
{
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
										  HandleCommand(line);
									  }
									  else
									  {
										  m_socket.close();
									  }
								  });
}

bool ImapSession::RequiresAuth(ImapCommandType type) const
{
	switch (type)
	{
	case ImapCommandType::Login:
	case ImapCommandType::Logout:
	case ImapCommandType::Capability:
	case ImapCommandType::Noop:
		return false;
	default:
		return true;
	}
}

void ImapSession::HandleCommand(const std::string& line)
{
	auto cmd = ImapParser::Parse(line);

	if (cmd.m_type == ImapCommandType::Unknown)
	{
		WriteResponse(cmd.m_tag + " BAD Command not recognized\r\n");
		ReadCommand();
		return;
	}

	if (RequiresAuth(cmd.m_type) && m_state == SessionState::NonAuthenticated)
	{
		WriteResponse(cmd.m_tag + " BAD Not authenticated\r\n");
		ReadCommand();
		return;
	}

	auto it = m_handlers.find(cmd.m_type);
	if (it != m_handlers.end())
	{
		auto response = it->second(cmd);
		WriteResponse(response);
	}
	else
	{
		WriteResponse(cmd.m_tag + " BAD Command not implemented\r\n");
	}

	if (cmd.m_type != ImapCommandType::Logout)
	{
		ReadCommand();
	}
}

void ImapSession::WriteResponse(const std::string& msg)
{
	auto self = shared_from_this();
	boost::asio::async_write(m_socket, boost::asio::buffer(msg), [this, self](std::error_code, std::size_t) {});
}

std::string ImapSession::HandleLogin(const ImapCommand& cmd)
{
	if (cmd.m_args.size() < 2)
	{
		return cmd.m_tag + " BAD Missing arguments\r\n";
	}

	return cmd.m_tag + " NO Login failed\r\n";
}

std::string ImapSession::HandleLogout(const ImapCommand& cmd)
{
	m_state = SessionState::NonAuthenticated;
	m_authenticatedUser.clear();
	m_currentMailbox = {};
	return "* BYE Logout completed\r\n" + cmd.m_tag + " OK Logout completed\r\n";
}

std::string ImapSession::HandleCapability(const ImapCommand& cmd)
{
	return ImapResponse::Capability() + cmd.m_tag + " OK Capability completed\r\n";
}

std::string ImapSession::HandleNoop(const ImapCommand& cmd)
{
	return cmd.m_tag + " OK Noop completed\r\n";
}

std::string ImapSession::HandleSelect(const ImapCommand& cmd)
{
	if (cmd.m_args.empty())
	{
		return cmd.m_tag + " BAD Missing mailbox name\r\n";
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
	return response;
}

std::string ImapSession::HandleList(const ImapCommand& cmd)
{
	std::string response = ImapResponse::Untagged("LIST (\\HasNoChildren) \"/\" \"INBOX\"");
	response += cmd.m_tag + " OK List completed\r\n";
	return response;
}

std::string ImapSession::HandleLsub(const ImapCommand& cmd)
{
	std::string response = ImapResponse::Untagged("LSUB (\\HasNoChildren) \"/\" \"INBOX\"");
	response += cmd.m_tag + " OK Lsub completed\r\n";
	return response;
}

std::string ImapSession::HandleStatus(const ImapCommand& cmd)
{
	if (cmd.m_args.empty())
	{
		return cmd.m_tag + " BAD Missing mailbox name\r\n";
	}

	std::string response = ImapResponse::Status(cmd.m_args[0], 100, 0, 5);
	response += cmd.m_tag + " OK Status completed\r\n";
	return response;
}

std::string ImapSession::HandleFetch(const ImapCommand& cmd)
{
	if (m_state != SessionState::Selected)
	{
		return cmd.m_tag + " BAD No mailbox selected\r\n";
	}

	if (cmd.m_args.size() < 2)
	{
		return cmd.m_tag + " BAD Missing arguments\r\n";
	}

	std::string response = ImapResponse::Fetch(1, "(FLAGS (\\Seen) RFC822.TEXT {4}\r\ntest)");
	response += cmd.m_tag + " OK Fetch completed\r\n";
	return response;
}

std::string ImapSession::HandleStore(const ImapCommand& cmd)
{
	if (m_state != SessionState::Selected)
	{
		return cmd.m_tag + " BAD No mailbox selected\r\n";
	}

	if (cmd.m_args.size() < 3)
	{
		return cmd.m_tag + " BAD Missing arguments\r\n";
	}

	return cmd.m_tag + " OK Store completed\r\n";
}

std::string ImapSession::HandleCreate(const ImapCommand& cmd)
{
	if (cmd.m_args.empty())
	{
		return cmd.m_tag + " BAD Missing mailbox name\r\n";
	}

	return cmd.m_tag + " OK Create completed\r\n";
}

std::string ImapSession::HandleDelete(const ImapCommand& cmd)
{
	if (cmd.m_args.empty())
	{
		return cmd.m_tag + " BAD Missing mailbox name\r\n";
	}

	return cmd.m_tag + " OK Delete completed\r\n";
}

std::string ImapSession::HandleRename(const ImapCommand& cmd)
{
	if (cmd.m_args.size() < 2)
	{
		return cmd.m_tag + " BAD Missing arguments\r\n";
	}

	return cmd.m_tag + " OK Rename completed\r\n";
}

std::string ImapSession::HandleCopy(const ImapCommand& cmd)
{
	if (m_state != SessionState::Selected)
	{
		return cmd.m_tag + " BAD No mailbox selected\r\n";
	}

	if (cmd.m_args.size() < 2)
	{
		return cmd.m_tag + " BAD Missing arguments\r\n";
	}

	return cmd.m_tag + " OK Copy completed\r\n";
}

std::string ImapSession::HandleExpunge(const ImapCommand& cmd)
{
	if (m_state != SessionState::Selected)
	{
		return cmd.m_tag + " BAD No mailbox selected\r\n";
	}

	return cmd.m_tag + " OK Expunge completed\r\n";
}
