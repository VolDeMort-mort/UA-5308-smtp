#include "SmtpParser.hpp"

#include <algorithm>

namespace
{

std::string toUpper(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::toupper(c); });
	return str;
}

bool startsWith(const std::string& str, const std::string& prefix)
{
	return str.rfind(prefix, 0) == 0;
}

} // namespace

SmtpCommand SmtpParser::parse(const std::string& line)
{
	SmtpCommand cmd;

	std::string upper = toUpper(line);

	if (startsWith(upper, "HELO "))
	{
		cmd.type = SmtpCommandType::HELO;
		cmd.argument = line.substr(5);
	}
	else if (startsWith(upper, "EHLO "))
	{
		cmd.type = SmtpCommandType::EHLO;
		cmd.argument = line.substr(5);
	}
	else if (startsWith(upper, "MAIL FROM:"))
	{
		cmd.type = SmtpCommandType::MAIL;
		cmd.argument = line.substr(10);
	}
	else if (startsWith(upper, "RCPT TO:"))
	{
		cmd.type = SmtpCommandType::RCPT;
		cmd.argument = line.substr(8);
	}
	else if (upper == "DATA")
	{
		cmd.type = SmtpCommandType::DATA;
	}
	else if (upper == "RSET")
	{
		cmd.type = SmtpCommandType::RSET;
	}
	else if (upper == "NOOP")
	{
		cmd.type = SmtpCommandType::NOOP;
	}
	else if (upper == "QUIT")
	{
		cmd.type = SmtpCommandType::QUIT;
	}

	return cmd;
}
