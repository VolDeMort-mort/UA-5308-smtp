#pragma once
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <string>
#include <map>
#include <set>
#include <vector>

#include "ImapCommand.hpp"

struct MailboxState
{
	std::string name;
	size_t exists = 0;
	size_t recent = 0;
	size_t unseen = 0;
	std::set<std::string> flags;
	bool readOnly = false;
};

class IMAP_Session : public std::enable_shared_from_this<IMAP_Session>
{
public:
	IMAP_Session(boost::asio::ip::tcp::socket socket);
	void start();

private:
	void sendBanner();
	void readCommand();
	void handleCommand(const std::string& line);
	void writeResponse(const std::string& msg);
	bool requiresAuth(ImapCommandType type) const;

	std::string handleLogin(const ImapCommand& cmd);
	std::string handleLogout(const ImapCommand& cmd);
	std::string handleCapability(const ImapCommand& cmd);
	std::string handleNoop(const ImapCommand& cmd);
	std::string handleSelect(const ImapCommand& cmd);
	std::string handleList(const ImapCommand& cmd);
	std::string handleLsub(const ImapCommand& cmd);
	std::string handleStatus(const ImapCommand& cmd);
	std::string handleFetch(const ImapCommand& cmd);
	std::string handleStore(const ImapCommand& cmd);
	std::string handleCreate(const ImapCommand& cmd);
	std::string handleDelete(const ImapCommand& cmd);
	std::string handleRename(const ImapCommand& cmd);
	std::string handleCopy(const ImapCommand& cmd);
	std::string handleExpunge(const ImapCommand& cmd);

	std::map<ImapCommandType, std::function<std::string(const ImapCommand&)>> handlers_;

	boost::asio::ip::tcp::socket socket_;
	boost::asio::streambuf buffer_;

	enum class State
	{
		NON_AUTH,
		AUTH,
		SELECTED
	} state_ = State::NON_AUTH;

	std::string authenticatedUser_;
	MailboxState currentMailbox_;
};
