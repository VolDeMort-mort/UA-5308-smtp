#pragma once

#include <boost/asio.hpp>

#include "DataBaseManager.h"
#include "ILogger.h"
#include "ImapConfig.hpp"
#include "ThreadPool.h"

class ImapServer
{
public:
	ImapServer(boost::asio::io_context& context, ILogger& m_logger, DataBaseManager& db, ThreadPool& pool,
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
