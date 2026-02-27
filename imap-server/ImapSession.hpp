#pragma once

#include <boost/asio.hpp>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "../logger/Logger.h"
#include "ImapCommand.hpp"

struct MailboxState
{
	std::string m_name;
	size_t m_exists = 0;
	size_t m_recent = 0;
	size_t m_unseen = 0;
	std::set<std::string> m_flags;
	bool m_readOnly = false;
};

enum class SessionState
{
	NonAuthenticated,
	Authenticated,
	Selected
};

class ImapSession : public std::enable_shared_from_this<ImapSession>
{
public:
	ImapSession(boost::asio::ip::tcp::socket socket, Logger& logger);
	void Start();

private:
	void SendBanner();
	void ReadCommand();
	void HandleCommand(const std::string& line);
	void WriteResponse(const std::string& msg);
	bool RequiresAuth(ImapCommandType type) const;

	std::string HandleLogin(const ImapCommand& cmd);
	std::string HandleLogout(const ImapCommand& cmd);
	std::string HandleCapability(const ImapCommand& cmd);
	std::string HandleNoop(const ImapCommand& cmd);
	std::string HandleSelect(const ImapCommand& cmd);
	std::string HandleList(const ImapCommand& cmd);
	std::string HandleLsub(const ImapCommand& cmd);
	std::string HandleStatus(const ImapCommand& cmd);
	std::string HandleFetch(const ImapCommand& cmd);
	std::string HandleStore(const ImapCommand& cmd);
	std::string HandleCreate(const ImapCommand& cmd);
	std::string HandleDelete(const ImapCommand& cmd);
	std::string HandleRename(const ImapCommand& cmd);
	std::string HandleCopy(const ImapCommand& cmd);
	std::string HandleExpunge(const ImapCommand& cmd);

	std::map<ImapCommandType, std::function<std::string(const ImapCommand&)>> m_handlers;

	boost::asio::ip::tcp::socket m_socket;
	boost::asio::streambuf m_buffer;
	Logger& m_logger;

	SessionState m_state = SessionState::NonAuthenticated;
	std::string m_authenticatedUser;
	MailboxState m_currentMailbox;
};
