#include "ImapSession.hpp"

#include "ImapParser.hpp"

ImapSession::ImapSession(boost::asio::ip::tcp::socket socket, ILogger& logger, DataBaseManager& db, UserDAL& u_dal)
	: m_socket(std::move(socket)), m_logger(logger), m_mess_repo(db), m_user_repo(db.getDB(), u_dal)
{
	m_logger.Log(PROD, "New ImapSession created");
	m_logger.Log(TRACE, "ImapSession::ImapSession - socket accepted");

	m_dispatcher = std::make_unique<ImapCommandDispatcher>(m_logger, m_user_repo, m_mess_repo);
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

	if (m_dispatcher->RequiresAuth(cmd.m_type) && m_dispatcher->get_State() == SessionState::NonAuthenticated)
	{
		m_logger.Log(PROD, "ImapSession::HandleCommand - User not authenticated");
		std::string response = cmd.m_tag + " BAD Not authenticated\r\n";
		WriteResponse(response);
		m_logger.Log(TRACE, "ImapSession::HandleCommand - Out: " + response);
		m_logger.Log(DEBUG, "ImapSession::HandleCommand - End");
		ReadCommand();
		return;
	}

	auto response = m_dispatcher->Dispatch(cmd);
	WriteResponse(response);

	m_logger.Log(TRACE, "ImapSession::HandleCommand - Out: " + response);
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
