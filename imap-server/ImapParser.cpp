#include "ImapParser.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>

#include "ImapUtils.hpp"

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

	cmd.m_args = IMAP_UTILS::SplitArgs(argsStr);
	cmd.m_type = IMAP_UTILS::StringToCommandType(IMAP_UTILS::ToUpper(commandStr));

	return cmd;
}