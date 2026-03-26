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

    inline std::string Ehlo(const std::string& domain, bool tls_active)
	{
		std::string r = "250-" + domain + "\r\n";
		if (!tls_active) r += "250-STARTTLS\r\n";
		r += "250 AUTH LOGIN PLAIN\r\n";
		return r;
	}

    inline std::string StartMailInput()
    {
        return "354 End data with <CR><LF>.<CR><LF>\r\n";
    }

	inline std::string MessageStorageFailed()
	{
		return "452 Message storage failed\r\n";
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
		return "250-" + domain + " STARTTLS\r\n";
	}

    inline std::string AuthChallenge(const std::string& b64)
	{
		return "334 " + b64 + "\r\n";
	}
	inline std::string AuthSucceeded()
	{
		return "235 Authentication successful\r\n";
	}
	inline std::string AuthFailed()
	{
		return "535 Authentication credentials invalid\r\n";
	}
	inline std::string AuthRequired()
	{
		return "530 Authentication required\r\n";
	}
	inline std::string EncryptionRequired()
	{
		return "538 Encryption required for requested auth mechanism\r\n";
	}

	inline std::string UnrecognizedAuthMech()
	{
		return "504 Unrecognized authentication mechanism\r\n";
	}
	}
