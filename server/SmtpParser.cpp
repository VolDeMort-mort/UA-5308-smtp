#include <algorithm>
#include <cctype>

#include "SmtpParser.hpp"

namespace
{
    std::string ToUpper(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char c){ return std::toupper(c); });
        return value;
    }

    bool StartsWith(const std::string& value,
                    const std::string& prefix)
    {
        return value.rfind(prefix, 0) == 0;
    }
}

SmtpCommand SmtpParser::Parse(const std::string& line)
{
    SmtpCommand command;

	std::string clean = line;

	if (!clean.empty() && clean.back() == '\r')
		clean.pop_back();

    std::string upper = ToUpper(clean);

    if (StartsWith(upper, "HELO "))
    {
        command.type = SmtpCommandType::HELO;
        command.argument = clean.substr(5);
    }
    else if (StartsWith(upper, "EHLO "))
    {
        command.type = SmtpCommandType::EHLO;
        command.argument = clean.substr(5);
    }
    else if (StartsWith(upper, "MAIL FROM:"))
    {
        command.type = SmtpCommandType::MAIL;
        command.argument = clean.substr(10);
    }
    else if (StartsWith(upper, "RCPT TO:"))
    {
        command.type = SmtpCommandType::RCPT;
        command.argument = clean.substr(8);
    }
    else if (upper == "DATA")
    {
        command.type = SmtpCommandType::DATA;
    }
    else if (upper == "RSET")
    {
        command.type = SmtpCommandType::RSET;
    }
    else if (upper == "NOOP")
    {
        command.type = SmtpCommandType::NOOP;
    }
    else if (upper == "QUIT")
    {
        command.type = SmtpCommandType::QUIT;
    }
	else if (upper == "STARTTLS")
	{
		command.type = SmtpCommandType::STARTTLS;
	}

    return command;
}
