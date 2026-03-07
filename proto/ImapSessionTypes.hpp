#pragma once

#include <cstdint>
#include <optional>
#include <set>
#include <string>

struct MailboxState
{
	std::string m_name;
	size_t m_exists = 0;
	size_t m_recent = 0;
	size_t m_unseen = 0;
	std::set<std::string> m_flags;
	std::optional<int64_t> m_id; // folder`s id in db
	bool m_readOnly = false;
};

enum class SessionState
{
	NonAuthenticated,
	Authenticated,
	Selected
};
