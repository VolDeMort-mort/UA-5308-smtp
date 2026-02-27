#include "ImapParser.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>

namespace
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

} // namespace

ImapCommand ImapParser::Parse(const std::string& line)
{
	ImapCommand cmd;

	if (line.empty())
	{
		return cmd;
	}

	size_t spacePos = line.find(' ');
	if (spacePos == std::string::npos)
	{
		cmd.m_tag = "";
		cmd.m_type = ImapCommandType::Unknown;
		return cmd;
	}

	cmd.m_tag = line.substr(0, spacePos);
	std::string rest = line.substr(spacePos + 1);

	size_t argStart = rest.find(' ');
	std::string commandStr;
	std::string argsStr;

	if (argStart == std::string::npos)
	{
		commandStr = rest;
		argsStr = "";
	}
	else
	{
		commandStr = rest.substr(0, argStart);
		argsStr = rest.substr(argStart + 1);
	}

	cmd.m_args = SplitArgs(argsStr);
	cmd.m_type = StringToCommandType(ToUpper(commandStr));

	return cmd;
}