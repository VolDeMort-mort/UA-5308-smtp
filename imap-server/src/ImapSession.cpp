#include "ImapSession.hpp"

#include "ImapParser.hpp"
#include "ImapResponse.hpp"

ImapSession::ImapSession(boost::asio::ip::tcp::socket socket, ILogger& logger, DataBaseManager& db, UserDAL& u_dal,
						 ThreadPool& pool)
	: m_socket(std::move(socket)), m_logger(logger), m_mess_repo(db), m_user_repo(db.getDB(), u_dal),
	  m_thread_pool(pool), m_strand(boost::asio::make_strand(m_socket.get_executor())), m_timer(socket.get_executor())
{
	m_logger.Log(PROD, "New ImapSession created");
	m_logger.Log(TRACE, "ImapSession::ImapSession - socket accepted");

	m_dispatcher = std::make_unique<ImapCommandDispatcher>(m_logger, m_user_repo, m_mess_repo);
}

void ImapSession::Start()
{
	m_logger.Log(DEBUG, "ImapSession::Start - Start");
	boost::asio::post(m_strand, [this, self = shared_from_this()]() { SendBanner(); });
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

	m_timer.expires_after(IMAP_TIMEOUT);
	m_timer.async_wait(
		boost::asio::bind_executor(m_strand,
								   [this, self](boost::system::error_code ec)
								   {
									   // the timer was cancelled after session received new command
									   if (ec == boost::asio::error::operation_aborted || !m_socket.is_open())
									   {
										   return;
									   }
									   WriteResponse(ImapResponse::Untagged("BYE Autologout; idle for too long"));
									   m_logger.Log(DEBUG, "Closing socket due to timeout");
									   m_socket.close();
								   }));

	boost::asio::async_read_until(
		m_socket, m_buffer, "\r\n",
		boost::asio::bind_executor(m_strand,
								   [this, self](boost::system::error_code ec, std::size_t)
								   {
									   if (!ec)
									   {
										   m_timer.cancel();

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
								   }));
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

	auto self = shared_from_this();

	m_thread_pool.add_task(
		[this, self, cmd]()
		{
			auto response = m_dispatcher->Dispatch(cmd);
			boost::asio::post(m_strand,
							  [this, self, response, cmd_type = cmd.m_type]()
							  {
								  WriteResponse(response);

								  if (cmd_type != ImapCommandType::Logout)
								  {
									  ReadCommand();
								  }
							  });
		});

	m_logger.Log(DEBUG, "ImapSession::HandleCommand - End");
}

void ImapSession::WriteResponse(const std::string& msg)
{
	m_logger.Log(TRACE, "ImapSession::WriteResponse - In: msg length=" + std::to_string(msg.size()));
	m_logger.Log(DEBUG, "ImapSession::WriteResponse - Start");

	m_write_queue.push(msg);
	if (!m_is_writing)
	{
		Write();
	}

	m_logger.Log(DEBUG, "ImapSession::WriteResponse - End");
}

void ImapSession::Write()
{
	m_logger.Log(DEBUG, "ImapSession::Write - Start");

	m_is_writing = true;
	auto self = shared_from_this();
	auto payload = std::make_shared<std::string>(m_write_queue.front());

	boost::asio::async_write(
		m_socket, boost::asio::buffer(*payload),
		boost::asio::bind_executor(m_strand,
								   [this, self, payload](boost::system::error_code ec, std::size_t bytes_transferred)
								   {
									   if (ec)
									   {
										   m_logger.Log(PROD, "ImapSession::Write - Error: " + ec.message());
										   m_socket.close();
										   return;
									   }

									   m_logger.Log(DEBUG, "ImapSession::Write - Sent " +
															   std::to_string(bytes_transferred) + " bytes");

									   m_write_queue.pop();
									   if (!m_write_queue.empty())
									   {
										   Write();
									   }
									   else
									   {
										   m_is_writing = false;
									   }
								   }));

	m_logger.Log(DEBUG, "ImapSession::Write - End");
}
