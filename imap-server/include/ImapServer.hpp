#pragma once

#include <boost/asio.hpp>

#include "AppConfig.h"
#include "DataBaseManager.h"
#include "ILogger.h"
#include "ThreadPool.h"

using namespace SmtpClient;

class ImapServer
{
public:
	ImapServer(boost::asio::io_context& context, ILogger& logger, DataBaseManager& db, ThreadPool& pool,
			   ImapConfig& config);
	ImapServer(const ImapServer&) = delete;
	void Start();

private:
	void AcceptConnection();

	ImapConfig& m_config;
	boost::asio::io_context& m_context;
	boost::asio::ip::tcp::acceptor m_acceptor;
	ILogger& m_logger;
	DataBaseManager& m_db;
	ThreadPool& m_thread_pool;
};
