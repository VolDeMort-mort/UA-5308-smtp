#include "ImapParser.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_map>

#include "ImapUtils.hpp"

std::vector<std::string> ImapParser::ParseArguments(const std::string& args)
{
	ElementParser parser(args);
	return parser.ParseArgs();
}

std::vector<std::string> ImapParser::ElementParser::ParseArgs()
{
	std::vector<std::string> args;

	while (m_pos < m_str.size())
	{
		SkipWhitespace();
		if (m_pos >= m_str.size()) break;

		size_t elemStart = m_pos;
		auto elem = ParseElement();

		if (m_pos > elemStart)
		{
			args.push_back(elem);
		}

		SkipWhitespace();
		if (m_pos >= m_str.size()) break;
	}

	return args;
}

void ImapParser::ElementParser::SkipWhitespace()
{
	while (m_pos < m_str.size() && m_str[m_pos] == ' ')
		++m_pos;
}

std::string ImapParser::ElementParser::ParseElement()
{
	if (m_pos >= m_str.size()) return "";

	char c = m_str[m_pos];
	if (c == '"') return ParseQuoted();
	if (c == '(') return ParseList();
	if (c == '[') return ParseResponse();
	if (c == '{') return ParseLiteral();

	return ParseAtom();
}

std::string ImapParser::ElementParser::ParseQuoted()
{
	if (m_pos >= m_str.size() || m_str[m_pos] != '"') return "";

	++m_pos;
	std::string result;
	while (m_pos < m_str.size())
	{
		char c = m_str[m_pos];
		if (c == '"')
		{
			++m_pos;
			break;
		}

		if (c == '\\' && m_pos + 1 < m_str.size())
		{
			++m_pos;
			result += m_str[m_pos++];
		}
		else
		{
			result += c;
			++m_pos;
		}
	}

	return result;
}

std::string ImapParser::ElementParser::ParseList()
{
	if (m_pos >= m_str.size() || m_str[m_pos] != '(') return "";

	++m_pos;
	int depth = 1;
	std::string result = "(";

	while (m_pos < m_str.size() && depth > 0)
	{
		char c = m_str[m_pos];
		if (c == '(')
		{
			++depth;
			result += c;
			++m_pos;
		}
		else if (c == ')')
		{
			--depth;
			result += c;
			++m_pos;
		}
		else if (c == '[')
		{
			result += c;
			++m_pos;
			int bracketDepth = 1;
			while (m_pos < m_str.size() && bracketDepth > 0)
			{
				char bc = m_str[m_pos];
				if (bc == '[')
					++bracketDepth;
				else if (bc == ']')
					--bracketDepth;
				result += bc;
				++m_pos;
			}
		}
		else if (c == '"')
		{
			std::string quoted = ParseQuoted();
			result += '"' + quoted + '"';
		}
		else
		{
			result += c;
			++m_pos;
		}
	}

	return result;
}

std::string ImapParser::ElementParser::ParseResponse()
{
	if (m_pos >= m_str.size() || m_str[m_pos] != '[') return "";

	++m_pos;
	int depth = 1;
	std::string result = "[";

	while (m_pos < m_str.size() && depth > 0)
	{
		char c = m_str[m_pos];
		if (c == '[')
		{
			++depth;
			result += c;
			++m_pos;
		}
		else if (c == ']')
		{
			--depth;
			result += c;
			++m_pos;
		}
		else if (c == '(')
		{
			result += c;
			++m_pos;
			int parenDepth = 1;
			while (m_pos < m_str.size() && parenDepth > 0)
			{
				char pc = m_str[m_pos];
				if (pc == '(')
					++parenDepth;
				else if (pc == ')')
					--parenDepth;
				result += pc;
				++m_pos;
			}
		}
		else if (c == '"')
		{
			std::string quoted = ParseQuoted();
			result += '"' + quoted + '"';
		}
		else
		{
			result += c;
			++m_pos;
		}
	}

	return result;
}

std::string ImapParser::ElementParser::ParseLiteral()
{
	if (m_pos >= m_str.size() || m_str[m_pos] != '{') return "";

	size_t start = m_pos;
	++m_pos;
	while (m_pos < m_str.size() && m_str[m_pos] != '}')
	{
		++m_pos;
	}
	if (m_pos < m_str.size())
	{
		++m_pos;
	}

	return m_str.substr(start, m_pos - start);
}

std::string ImapParser::ElementParser::ParseAtom()
{
	size_t start = m_pos;
	while (m_pos < m_str.size())
	{
		char c = m_str[m_pos];
		if (c == ' ' || c == '(' || c == ')' || c == '[' || c == ']' || c == '"' || c == '{') break;
		++m_pos;
	}

	return m_str.substr(start, m_pos - start);
}

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

	std::string upperCommand = IMAP_UTILS::ToUpper(commandStr);

	if (upperCommand == "UID")
	{
		cmd.m_type = IMAP_UTILS::StringToCommandType("UID");
		IMAP_UTILS::TrimParentheses(argsStr);
		std::string uidArgs = argsStr;
		size_t spaceInUidArgs = uidArgs.find(' ');
		if (spaceInUidArgs != std::string::npos)
		{
			std::string subCommand = uidArgs.substr(0, spaceInUidArgs);
			std::string restArgs = uidArgs.substr(spaceInUidArgs + 1);

			std::string upperSub = IMAP_UTILS::ToUpper(subCommand);
			if (upperSub == "FETCH")
			{
				cmd.m_type = ImapCommandType::UidFetch;
				cmd.m_args = ParseArguments(restArgs);
			}
			else if (upperSub == "STORE")
			{
				cmd.m_type = ImapCommandType::UidStore;
				cmd.m_args = ParseArguments(restArgs);
			}
			else if (upperSub == "COPY")
			{
				cmd.m_type = ImapCommandType::UidCopy;
				cmd.m_args = ParseArguments(restArgs);
			}
			else
			{
				cmd.m_type = ImapCommandType::Unknown;
			}
		}
		else
		{
			cmd.m_type = ImapCommandType::Unknown;
		}
	}
	else
	{
		cmd.m_args = ParseArguments(argsStr);
		cmd.m_type = IMAP_UTILS::StringToCommandType(commandStr);
	}

	return cmd;
}
