#pragma once

#include <string>

namespace SmtpResponse
{
    inline std::string ServiceReady(const std::string& domain)
    {
        return "220 " + domain + " Service ready\r\n";
    }

    inline std::string Ok()
    {
        return "250 OK\r\n";
    }

    inline std::string Hello(const std::string& domain)
    {
        return "250 " + domain + " greets you\r\n";
    }

    inline std::string StartMailInput()
    {
        return "354 End data with <CR><LF>.<CR><LF>\r\n";
    }

    inline std::string BadSequence()
    {
        return "503 Bad sequence of commands\r\n";
    }

	inline std::string MessageTooLarge()
	{
		return "552 Message size exceeds fixed maximum\r\n";
	}

    inline std::string SyntaxError()
    {
        return "500 Syntax error\r\n";
    }

    inline std::string NotImplemented()
    {
        return "502 Command not implemented\r\n";
    }

    inline std::string Closing()
    {
        return "221 Bye\r\n";
    }
	inline std::string StartTls()
	{
		return "220 Ready to start TLS\r\n";
	}

	inline std::string ReadyToStartTLS(const std::string& domain)
	{
		return "250 " + domain + " STARTTLS\r\n";
	}

	}
