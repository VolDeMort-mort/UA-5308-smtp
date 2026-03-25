#pragma once

#include <string>
#include <vector>

enum class ImapCommandType
{
	Login,
	Logout,
	Capability,
	Noop,
	Select,
	List,
	Lsub,
	Status,
	Fetch,
	Store,
	Create,
	Delete,
	Rename,
	Copy,
	Expunge,
	UidFetch,
	UidStore,
	UidCopy,
	Subscribe,
	Unsubscribe,
	Close,
	Check,
	Unknown
};

struct ImapCommand
{
	std::string m_tag;
	ImapCommandType m_type = ImapCommandType::Unknown;
	std::vector<std::string> m_args;
};
