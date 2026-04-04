#include "ImapSession.hpp"

#include <istream>

#include "ImapParser.hpp"
#include "ImapResponse.hpp"

ImapSession::ImapSession(boost::asio::ip::tcp::socket socket, ILogger& logger, DataBaseManager& db,
						 ThreadPool& pool, ImapConfig& config)
	: m_config(config), m_socket(std::move(socket)), m_logger(logger), m_mess_repo(db), m_user_repo(db),
	  m_conn(m_socket), m_secure_channel(std::make_unique<ServerSecureChannel>(m_conn)), 
	  m_thread_pool(pool), m_strand(boost::asio::make_strand(m_socket.get_executor())), m_timer(m_socket.get_executor())
{
	m_logger.Log(PROD, "New ImapSession created");
	m_logger.Log(TRACE, "ImapSession::ImapSession - socket accepted");

	m_dispatcher = std::make_unique<ImapCommandDispatcher>(m_logger, m_user_repo, m_mess_repo);
	m_secure_channel->setLogger(&m_logger);
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

	m_timer.expires_after(std::chrono::minutes(m_config.TIMEOUT_MINS));
	m_timer.async_wait(
		boost::asio::bind_executor(m_strand,
								   [this, self](boost::system::error_code ec)
								   {
									   // the timer was cancelled after session received new command
									   if (ec == boost::asio::error::operation_aborted || !m_socket.is_open())
									   {
										   return;
									   }
									   m_closing = true;
									   WriteResponse(ImapResponse::Untagged("BYE Autologout; idle for too long"));
									   m_logger.Log(DEBUG, "Closing socket due to timeout");
								   }));

	if (m_secure_channel->isSecure())
	{
		m_thread_pool.add_task(
			[this, self]()
			{
				std::string line;
				bool ok = m_secure_channel->Receive(line);

				boost::asio::post(m_strand,
								  [this, self, ok, line]()
								  {
									  if (ok)
									  {
										  m_timer.cancel();
										  HandleCommand(line);
									  }
									  else
									  {
										  m_logger.Log(PROD, "IMAP: Secure Receive failed");
										  m_socket.close();
									  }
								  });
			});
	}

	else
	{
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
										   
									   }));
	}

	m_logger.Log(DEBUG, "ImapSession::ReadCommand - End");
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
			std::string response;
			try
			{
				response = m_dispatcher->Dispatch(cmd);
			}
			catch (const std::exception& ex)
			{
				m_logger.Log(PROD, std::string("Exception in command dispatch: ") + ex.what());
				response = "BAD Internal server error\r\n";
			}
			boost::asio::post(
				m_strand,
				[this, self, response, cmd]()
				{
					if (cmd.m_type == ImapCommandType::StartTLS && m_secure_channel->isSecure())
					{
						m_logger.Log(
							DEBUG,
							"STARTTLS command received when TLS is invoked already. Changing response result to BAD");
						WriteResponse(cmd.m_tag + " BAD TLS already active\r\n");
						ReadCommand();
					}
					else if (cmd.m_type == ImapCommandType::StartTLS)
					{
						m_is_starttls_pending = true;
						m_logger.Log(DEBUG, "STARTTLS response queued. Waiting for Write() to finish.");
						WriteResponse(response);
					}
					else
					{
						WriteResponse(response);
						if (cmd.m_type != ImapCommandType::Logout)
						{
							ReadCommand();
						}
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

	auto write_handler = [this, self, payload](boost::system::error_code ec, std::size_t bytes_transferred)
	{
		if (ec)
		{
			m_logger.Log(PROD, "ImapSession::Write - Error: " + ec.message());
			m_socket.close();
			return;
		}

		m_logger.Log(DEBUG, "ImapSession::Write - Sent " + std::to_string(bytes_transferred) + " bytes");

		m_write_queue.pop();
		if (!m_write_queue.empty())
		{
			Write();
		}
		else
		{
			m_is_writing = false;

			if (m_closing) 
			{
				m_socket.close();
				return;
			}

			if (m_is_starttls_pending)
			{
				m_is_starttls_pending = false;
				m_logger.Log(PROD, "STARTTLS response physically sent. Triggering Handshake!");

				UpgradeToTLS();
			}
		}
	};

	if (m_secure_channel->isSecure())
	{
		m_thread_pool.add_task(
			[this, self, payload]()
			{
				bool ok = m_secure_channel->Send(*payload);

				boost::asio::post(m_strand,
								  [this, self, ok]()
								  {
									  if (!ok)
									  {
										  m_logger.Log(PROD, "Secure send failed");
										  m_socket.close();
										  return;
									  }
									  m_write_queue.pop();
									  if (!m_write_queue.empty())
									  {
										  Write();
									  }
									  else
									  {
										  m_is_writing = false;

										  if (m_closing)
										  {
											  m_socket.close();
											  return;
										  }

										  if (m_is_starttls_pending)
										  {
											  m_is_starttls_pending = false;
											  UpgradeToTLS();
										  }
									  }
								  });
			});
	}
	else
	{
		boost::asio::async_write(m_socket, boost::asio::buffer(*payload),
								 boost::asio::bind_executor(m_strand, write_handler));
	}

	m_logger.Log(DEBUG, "ImapSession::Write - End");
}

void ImapSession::UpgradeToTLS()
{
	m_timer.cancel();

	m_timer.expires_after(std::chrono::seconds(m_config.HANDSHAKE_TIMEOUT_SECS));
	m_timer.async_wait(boost::asio::bind_executor(m_strand,
												  [this, self = shared_from_this()](boost::system::error_code ec)
												  {
													  if (!ec)
													  {
														  m_logger.Log(PROD, "IMAP: TLS handshake timeout");
														  m_socket.close(); 
													  }
												  }));

	auto self = shared_from_this();
	m_thread_pool.add_task(
		[this, self]()
		{
			bool ok = m_secure_channel->StartTLS();

			boost::asio::post(m_strand,
							  [this, self, ok]()
							  {
								  m_timer.cancel();
								  if (ok)
								  {
									  m_logger.Log(PROD, "IMAP: TLS handshake completed");
									  ReadCommand();
								  }
								  else
								  {
									  m_logger.Log(PROD, "IMAP: TLS handshake failed");
									  m_socket.close();
								  }
							  });
		});
}