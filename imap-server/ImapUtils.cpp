#include "ImapUtils.hpp"

#include <algorithm>
#include <sstream>
#include <unordered_map>

namespace IMAP_UTILS
{

std::string ToUpper(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::toupper(c); });
	return str;
}

std::vector<std::string> SplitArgs(const std::string& str)
{
	std::vector<std::string> args;
	std::istringstream iss(str);
	std::string arg;
	while (iss >> arg)
	{
		args.push_back(arg);
	}
	return args;
}

std::string JoinArgs(const std::vector<std::string>& args)
{
	if (args.empty()) return "";
	std::string result;
	for (size_t i = 0; i < args.size(); ++i)
	{
		result += args[i];
		if (i < args.size() - 1) result += ", ";
	}
	return result;
}

ImapCommandType StringToCommandType(const std::string& cmd)
{
	static const std::unordered_map<std::string, ImapCommandType> commandMap = {
		{"LOGIN", ImapCommandType::Login},
		{"LOGOUT", ImapCommandType::Logout},
		{"CAPABILITY", ImapCommandType::Capability},
		{"NOOP", ImapCommandType::Noop},
		{"SELECT", ImapCommandType::Select},
		{"LIST", ImapCommandType::List},
		{"LSUB", ImapCommandType::Lsub},
		{"STATUS", ImapCommandType::Status},
		{"FETCH", ImapCommandType::Fetch},
		{"STORE", ImapCommandType::Store},
		{"CREATE", ImapCommandType::Create},
		{"DELETE", ImapCommandType::Delete},
		{"RENAME", ImapCommandType::Rename},
		{"IDLE", ImapCommandType::Idle},
		{"COPY", ImapCommandType::Copy},
		{"MOVE", ImapCommandType::Move},
		{"EXPUNGE", ImapCommandType::Expunge},
	};

	auto it = commandMap.find(cmd);
	if (it != commandMap.end())
	{
		return it->second;
	}
	return ImapCommandType::Unknown;
}

std::string CommandTypeToString(const ImapCommandType& type)
{
	static const std::unordered_map<ImapCommandType, std::string> commandMap = {
		{ImapCommandType::Login, "LOGIN"},
		{ImapCommandType::Logout, "LOGOUT"},
		{ImapCommandType::Capability, "CAPABILITY"},
		{ImapCommandType::Noop, "NOOP"},
		{ImapCommandType::Select, "SELECT"},
		{ImapCommandType::List, "LIST"},
		{ImapCommandType::Lsub, "LSUB"},
		{ImapCommandType::Status, "STATUS"},
		{ImapCommandType::Fetch, "FETCH"},
		{ImapCommandType::Store, "STORE"},
		{ImapCommandType::Create, "CREATE"},
		{ImapCommandType::Delete, "DELETE"},
		{ImapCommandType::Rename, "RENAME"},
		{ImapCommandType::Idle, "IDLE"},
		{ImapCommandType::Copy, "COPY"},
		{ImapCommandType::Move, "MOVE"},
		{ImapCommandType::Expunge, "EXPUNGE"},
	};

	auto it = commandMap.find(type);
	if (it != commandMap.end())
	{
		return it->second;
	}
	return "UNKNOWN";
}

} // namespace IMAP_UTILS