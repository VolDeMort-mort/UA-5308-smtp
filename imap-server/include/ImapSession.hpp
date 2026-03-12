#pragma once

#include <boost/asio.hpp>
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
#include "ImapSessionTypes.hpp"
#include "Logger.h"
#include "Repository/MessageRepository.h"
#include "Repository/UserRepository.h"
#include "ThreadPool.h"

class ImapSession : public std::enable_shared_from_this<ImapSession>
{
public:
	ImapSession(boost::asio::ip::tcp::socket socket, ILogger& logger, DataBaseManager& db, UserDAL& u_dal,
				ThreadPool& pool);
	void Start();

private:
	void SendBanner();
	void ReadCommand();
	void HandleCommand(const std::string& line);
	void WriteResponse(const std::string& msg); // adds message to the queue
	void Write();								// writes to the client from queue

	boost::asio::ip::tcp::socket m_socket;
	boost::asio::streambuf m_buffer;
	boost::asio::strand<boost::asio::any_io_executor> m_strand;

	std::queue<std::string> m_write_queue;
	bool m_is_writing = false;

	ILogger& m_logger;
	ThreadPool& m_thread_pool;

	MessageRepository m_mess_repo;
	UserRepository m_user_repo;

	std::unique_ptr<ImapCommandDispatcher> m_dispatcher;
};
