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

    std::string Trim(const std::string& s)
    {
        const auto begin = s.find_first_not_of(" \t\r\n");
        if (begin == std::string::npos) return {};
        const auto end = s.find_last_not_of(" \t\r\n");
        return s.substr(begin, end - begin + 1);
    }

    std::string StripAngleBrackets(const std::string& s)
    {
        if (s.size() >= 2 && s.front() == '<' && s.back() == '>')
            return s.substr(1, s.size() - 2);
        return s;
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
        command.argument = Trim(clean.substr(5));
    }
    else if (StartsWith(upper, "EHLO "))
    {
        command.type = SmtpCommandType::EHLO;
        command.argument = Trim(clean.substr(5));
    }
    else if (StartsWith(upper, "MAIL FROM:"))
    {
        command.type = SmtpCommandType::MAIL;
        command.argument = StripAngleBrackets(Trim(clean.substr(10)));
    }
    else if (StartsWith(upper, "RCPT TO:"))
    {
        command.type = SmtpCommandType::RCPT;
        command.argument = StripAngleBrackets(Trim(clean.substr(8)));
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
	else if (StartsWith(upper, "AUTH "))
	{
		command.type = SmtpCommandType::AUTH;
		
        std::string arg = clean.substr(5);

        // to insure that credentials that could be sent with AUTH stau case-sensative
		auto sp = arg.find(' ');
		if (sp == std::string::npos)
		{
			command.argument = ToUpper(arg);
		}
		else
		{
			command.argument = ToUpper(arg.substr(0, sp)) + " " + arg.substr(sp + 1);
		}
	}

    return command;
}
