#pragma once

#include <boost/asio.hpp>

#include "DataBaseManager.h"
#include "ILogger.h"

constexpr int IMAP_PORT = 2553;

class ImapServer
{
public:
	ImapServer(boost::asio::io_context& context, ILogger& m_logger, DataBaseManager& db);
	ImapServer(const ImapServer&) = delete;
	void Start();

private:
	void AcceptConnection();

	boost::asio::io_context& m_context;
	boost::asio::ip::tcp::acceptor m_acceptor;
	ILogger& m_logger;
	DataBaseManager& m_db;
};
