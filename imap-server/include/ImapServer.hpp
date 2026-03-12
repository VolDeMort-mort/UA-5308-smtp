#pragma once

#include <boost/asio.hpp>

#include "DAL/UserDAL.h"
#include "DataBaseManager.h"
#include "ILogger.h"
#include "ThreadPool.h"

constexpr int IMAP_PORT = 2553;

class ImapServer
{
public:
	ImapServer(boost::asio::io_context& context, ILogger& m_logger, DataBaseManager& db, ThreadPool& pool);
	ImapServer(const ImapServer&) = delete;
	void Start();

private:
	void AcceptConnection();

	boost::asio::io_context& m_context;
	boost::asio::ip::tcp::acceptor m_acceptor;
	ILogger& m_logger;
	DataBaseManager& m_db;
	UserDAL m_user_dal; // only here because of repos initialization, user repo`s signature should as message repo`s
	ThreadPool& m_thread_pool;
};
