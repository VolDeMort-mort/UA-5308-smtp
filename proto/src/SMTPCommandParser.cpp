#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>

#include "SMTPCommandParser.h"

static void trim(std::string& s)
{
	size_t start = 0;
	while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
	size_t end = s.size();
	while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
	s = s.substr(start, end - start);
}

void SMTPCommandParser::ParseLine(std::string& line, ClientCommand& cmd)
{
	cmd.command = CommandType::UNKNOWN;
	cmd.commandText.clear();

	if (!line.empty() && line.back() == '\n') line.pop_back();
	if (!line.empty() && line.back() == '\r') line.pop_back();

	std::istringstream iss(line);
	std::string verb;
	if (!(iss >> verb)) return;

	std::transform(verb.begin(), verb.end(), verb.begin(),
				   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

	if (verb == "MAIL" || verb == "RCPT")
	{
		std::string keyword;
		if (!(iss >> keyword)) return;

		auto pos_colon = keyword.find(':');
		std::string key_upper = keyword.substr(0, pos_colon);
		std::transform(key_upper.begin(), key_upper.end(), key_upper.begin(),
					   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

		std::string expected = (verb == "MAIL") ? "FROM" : "TO";
		if (key_upper != expected) return;

		std::string address;
		if (pos_colon != std::string::npos) address = keyword.substr(pos_colon + 1);

		std::string rest;
		std::getline(iss, rest);
		address += rest;
		trim(address);

		if (!address.empty() && address.front() == '<' && address.back() == '>')
			address = address.substr(1, address.size() - 2);

		if (!address.empty())
		{
			cmd.command = (verb == "MAIL") ? CommandType::MAIL : CommandType::RCPT;
			cmd.commandText = address;
		}
		return;
	}

	if (verb == "HELO")
	{
		cmd.command = CommandType::HELO;
	}
	else if (verb == "EHLO")
	{
		cmd.command = CommandType::EHLO;
	}
	else if (verb == "DATA")
	{
		cmd.command = CommandType::DATA;
	}
	else if (verb == "STARTTLS")
	{
		cmd.command = CommandType::STARTTLS;
	}
	else if (verb == "QUIT")
	{
		cmd.command = CommandType::QUIT;
	}

	std::string text;
	std::getline(iss, text);
	trim(text);
	cmd.commandText = text;
}
