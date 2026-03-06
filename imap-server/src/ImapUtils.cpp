#include "ImapUtils.hpp"

#include <algorithm>
#include <numeric>
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

	auto it = commandMap.find(IMAP_UTILS::ToUpper(cmd));
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

std::string TrimParentheses(const std::string& str)
{
	if (str.size() >= 2 && str.front() == '(' && str.back() == ')')
	{
		return str.substr(1, str.size() - 2);
	}
	return str;
}

std::vector<std::string> Split(const std::string& str, char delim)
{
	std::vector<std::string> result;
	std::stringstream ss(str);
	std::string token;

	while (std::getline(ss, token, delim))
	{
		result.push_back(token);
	}

	return result;
}

std::vector<int64_t> ParseSequenceSet(const std::string& sequenceSet, int64_t maxValue)
{
	std::vector<int64_t> res;
	try
	{
		int num = std::stoi(sequenceSet);
		res.push_back(num);
	}
	catch (const std::invalid_argument& e)
	{
		auto semicolon_index = sequenceSet.find(':');
		if (semicolon_index != std::string::npos)
		{
			auto firstPart = sequenceSet.substr(0, semicolon_index);
			auto secondPart = sequenceSet.substr(semicolon_index + 1);

			int64_t first, second;
			if (firstPart == "*")
			{
				swap(firstPart, secondPart);
			}
			if (secondPart == "*")
			{
				second = maxValue;
			}
			first = std::stoi(firstPart);
			res.resize(second - first + 1);
			std::iota(res.begin(), res.end(), first);
		}
		else
		{
			auto str_res = Split(sequenceSet, ',');
			res.resize(str_res.size());
			std::transform(str_res.begin(), str_res.end(), res.begin(),
						   [](const std::string& s) { return std::stoi(s); });
		}
	}

	return res;
}

void SortMessagesByTimeDescending(std::vector<Message>& messages)
{
	std::sort(messages.begin(), messages.end(),
			  [](const Message& a, const Message& b) { return a.created_at > b.created_at; });
}

} // namespace IMAP_UTILS
