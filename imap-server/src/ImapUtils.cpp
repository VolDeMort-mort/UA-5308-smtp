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

	auto parts = Split(sequenceSet, ',');

	for (const auto& part : parts)
	{
		size_t colon_pos = part.find(':');

		if (colon_pos != std::string::npos)
		{
			std::string firstPart = part.substr(0, colon_pos);
			std::string secondPart = part.substr(colon_pos + 1);

			int64_t first = (firstPart == "*") ? maxValue : std::stoll(firstPart);
			int64_t second = (secondPart == "*") ? maxValue : std::stoll(secondPart);

			if (first > second)
			{
				std::swap(first, second);
			}

			for (int64_t i = first; i <= second; ++i)
			{
				res.push_back(i);
			}
		}
		else
		{
			if (part == "*")
			{
				res.push_back(maxValue);
			}
			else
			{
				res.push_back(std::stoll(part));
			}
		}
	}

	std::sort(res.begin(), res.end());
	res.erase(std::unique(res.begin(), res.end()), res.end());

	return res;
}

void SortMessagesByTimeDescending(std::vector<Message>& messages)
{
	std::sort(messages.begin(), messages.end(),
			  [](const Message& a, const Message& b) { return a.created_at > b.created_at; });
}

} // namespace IMAP_UTILS
