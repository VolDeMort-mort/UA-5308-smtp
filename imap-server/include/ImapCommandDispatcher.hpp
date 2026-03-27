#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "ImapCommand.hpp"
#include "ImapSessionTypes.hpp"
#include "Logger.h"
#include "Repository/MessageRepository.h"
#include "Repository/UserRepository.h"

class ImapCommandDispatcher
{
public:
	ImapCommandDispatcher(ILogger& logger, UserRepository& userRepo, MessageRepository& messRepo);

	std::string Dispatch(const ImapCommand& cmd);
	bool RequiresAuth(ImapCommandType type) const;

	SessionState get_State() const { return m_state; }
	void set_State(SessionState state) { m_state = state; }

	std::optional<int64_t> get_AuthenticatedUserID() const { return m_authenticatedUserID; }
	const std::string& get_AuthenticatedUserName() const { return m_authenticatedUserName; }
	const MailboxState& get_MailboxState() const { return m_currentMailbox; }

private:
	ILogger& m_logger;
	UserRepository& m_userRepo;
	MessageRepository& m_messRepo;

	SessionState m_state = SessionState::NonAuthenticated;
	std::string m_authenticatedUserName;
	std::optional<int64_t> m_authenticatedUserID;
	MailboxState m_currentMailbox;

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
	std::string HandleUidFetch(const ImapCommand& cmd);
	std::string HandleUidStore(const ImapCommand& cmd);
	std::string HandleUidCopy(const ImapCommand& cmd);
	std::string HandleSubscribe(const ImapCommand& cmd);
	std::string HandleUnsubscribe(const ImapCommand& cmd);
	std::string HandleClose(const ImapCommand& cmd);
	std::string HandleCheck(const ImapCommand& cmd);

	std::map<ImapCommandType, std::function<std::string(const ImapCommand&)>> m_handlers;
};
