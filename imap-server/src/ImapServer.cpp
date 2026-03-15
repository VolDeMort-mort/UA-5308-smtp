#include "ImapServer.hpp"

#include <sstream>

#include "ImapSession.hpp"

ImapServer::ImapServer(boost::asio::io_context& context, ILogger& logger, DataBaseManager& db, ThreadPool& pool)
	: m_context(context), m_acceptor(context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), IMAP_PORT)),
	  m_logger(logger), m_db(db), m_user_dal(m_db.getDB()), m_thread_pool(pool)
{
	m_logger.Log(PROD, "Imap server entity created");
}

void ImapServer::Start()
{
	m_logger.Log(PROD, "Server started");
	AcceptConnection();
	m_context.run();
}

void ImapServer::AcceptConnection()
{
	m_logger.Log(DEBUG, "AcceptConnection called");
	m_acceptor.async_accept(
		[this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket)
		{
			if (ec == boost::asio::error::operation_aborted)
			{
				m_logger.Log(DEBUG, "Acceptor stopped");
				return;
			}

			if (!ec)
			{
				std::make_shared<ImapSession>(std::move(socket), m_logger, m_db, m_user_dal, m_thread_pool)->Start();
			}
			else
			{
				std::stringstream ss;
				ss << "Error: " << ec.message();
				m_logger.Log(PROD, ss.str());
			}
			AcceptConnection();
		});
}
