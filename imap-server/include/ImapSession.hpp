#pragma once

#include <boost/asio.hpp>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
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

class ImapSession : public std::enable_shared_from_this<ImapSession>
{
public:
	ImapSession(boost::asio::ip::tcp::socket socket, ILogger& logger, DataBaseManager& db);
	void Start();

private:
	void SendBanner();
	void ReadCommand();
	void HandleCommand(const std::string& line);
	void WriteResponse(const std::string& msg);

	boost::asio::ip::tcp::socket m_socket;
	boost::asio::streambuf m_buffer;
	ILogger& m_logger;

	MessageRepository m_mess_repo;
	UserRepository m_user_repo;

	std::unique_ptr<ImapCommandDispatcher> m_dispatcher;
};
