#pragma once
#include <string>

enum class SmtpCommandType
{
	HELO,
	EHLO,
	MAIL,
	RCPT,
	DATA,
	RSET,
	NOOP,
	QUIT,
	UNKNOWN
};

struct SmtpCommand
{
	SmtpCommandType type{SmtpCommandType::UNKNOWN};
	std::string argument;
};
