#pragma once
#include <string>
#include <vector>

enum class ImapCommandType
{
	LOGIN,
	LOGOUT,
	CAPABILITY,
	NOOP,
	SELECT,
	LIST,
	LSUB,
	STATUS,
	FETCH,
	STORE,
	CREATE,
	DELETE,
	RENAME,
	IDLE,
	COPY,
	MOVE,
	EXPUNGE,
	UNKNOWN
};

struct ImapCommand
{
	std::string tag;
	ImapCommandType type{ImapCommandType::UNKNOWN};
	std::vector<std::string> args;
};
