#include "ImapParser.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>

namespace
{

std::string toUpper(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::toupper(c); });
	return str;
}

std::vector<std::string> splitArgs(const std::string& str)
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

ImapCommandType stringToCommandType(const std::string& cmd)
{
	static const std::unordered_map<std::string, ImapCommandType> commandMap = {
		{"LOGIN", ImapCommandType::LOGIN},
		{"LOGOUT", ImapCommandType::LOGOUT},
		{"CAPABILITY", ImapCommandType::CAPABILITY},
		{"NOOP", ImapCommandType::NOOP},
		{"SELECT", ImapCommandType::SELECT},
		{"LIST", ImapCommandType::LIST},
		{"LSUB", ImapCommandType::LSUB},
		{"STATUS", ImapCommandType::STATUS},
		{"FETCH", ImapCommandType::FETCH},
		{"STORE", ImapCommandType::STORE},
		{"CREATE", ImapCommandType::CREATE},
		{"DELETE", ImapCommandType::DELETE},
		{"RENAME", ImapCommandType::RENAME},
		{"IDLE", ImapCommandType::IDLE},
		{"COPY", ImapCommandType::COPY},
		{"MOVE", ImapCommandType::MOVE},
		{"EXPUNGE", ImapCommandType::EXPUNGE},
	};

	auto it = commandMap.find(cmd);
	if (it != commandMap.end())
	{
		return it->second;
	}
	return ImapCommandType::UNKNOWN;
}

} // namespace

ImapCommand ImapParser::parse(const std::string& line)
{
	ImapCommand cmd;

	if (line.empty())
	{
		return cmd;
	}

	size_t spacePos = line.find(' ');
	if (spacePos == std::string::npos)
	{
		cmd.tag = "";
		cmd.type = ImapCommandType::UNKNOWN;
		return cmd;
	}

	cmd.tag = line.substr(0, spacePos);
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

	cmd.args = splitArgs(argsStr);
	cmd.type = stringToCommandType(toUpper(commandStr));

	return cmd;
}
