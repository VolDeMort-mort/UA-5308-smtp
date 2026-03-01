#include "SMTPCommandParser.h"
#include <string>
#include <cctype>
#include <sstream>
#include <algorithm>

void SMTPCommandParser::parseLine(std::string& line, ClientCommand& cmd)
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

	if (verb == "HELO")
	{
		cmd.command = CommandType::HELO;
		std::getline(iss, cmd.commandText);
	}
	else if (verb == "EHLO")
	{
		cmd.command = CommandType::EHLO;
		std::getline(iss, cmd.commandText);
	}
	else if (verb == "MAIL" || verb == "RCPT")
	{
		std::string keyword;
		if (!(iss >> keyword)) return;

		std::transform(keyword.begin(), keyword.end(), keyword.begin(),
					   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

		std::string prefix = (verb == "MAIL") ? "FROM:" : "TO:";
		if (keyword.rfind(prefix, 0) != 0) return;

		std::string address = keyword.substr(prefix.size());

		std::string rest;
		std::getline(iss, rest);
		
		address += rest;

		if (!address.empty() && address.front() != '<') address = "<" + address;
		if (!address.empty() && address.back() != '>') address += ">";

		cmd.command = (verb == "MAIL") ? CommandType::MAIL : CommandType::RCPT;
		cmd.commandText = address;
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
}
