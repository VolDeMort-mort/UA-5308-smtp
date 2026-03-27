#pragma once

#include <boost/asio.hpp>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <vector>

#include "DataBaseManager.h"
#include "ImapCommand.hpp"
#include "ImapCommandDispatcher.hpp"
#include "ImapConfig.hpp"
#include "ImapSessionTypes.hpp"
#include "Logger.h"
#include "Repository/MessageRepository.h"
#include "Repository/UserRepository.h"
#include "ThreadPool.h"
#include "ImapConnection.hpp"
#include "ServerSecureChannel.hpp"

class ImapSession : public std::enable_shared_from_this<ImapSession>
{
public:
	ImapSession(boost::asio::ip::tcp::socket socket, ILogger& logger, DataBaseManager& db, UserDAL& u_dal,
				ThreadPool& pool, ImapConfig& config);
	void Start();

private:
	void SendBanner();
	void ReadCommand();
	void HandleCommand(const std::string& line);
	void WriteResponse(const std::string& msg); // adds message to the queue
	void Write();								// writes to the client from queue
	void UpgradeToTLS();

	ImapConfig& m_config;
	boost::asio::ip::tcp::socket m_socket;
	ImapConnection m_conn;
	std::unique_ptr<ServerSecureChannel> m_secure_channel;
	boost::asio::streambuf m_buffer;
	boost::asio::strand<boost::asio::any_io_executor> m_strand;
	boost::asio::steady_timer m_timer;

	bool m_is_starttls_pending = false;

	std::queue<std::string> m_write_queue;
	bool m_is_writing = false;

	ILogger& m_logger;
	ThreadPool& m_thread_pool;

	MessageRepository m_mess_repo;
	UserRepository m_user_repo;

	std::unique_ptr<ImapCommandDispatcher> m_dispatcher;
};
