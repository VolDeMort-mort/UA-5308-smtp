#pragma once
#include <string>

namespace SmtpResponse
{

inline std::string serviceReady(const std::string& domain)
{
	return "220 " + domain + " Service ready\r\n";
}

inline std::string ok()
{
	return "250 OK\r\n";
}

inline std::string hello(const std::string& domain)
{
	return "250 " + domain + " greets you\r\n";
}

inline std::string startMailInput()
{
	return "354 End data with <CR><LF>.<CR><LF>\r\n";
}

inline std::string badSequence()
{
	return "503 Bad sequence of commands\r\n";
}

inline std::string syntaxError()
{
	return "500 Syntax error\r\n";
}

inline std::string commandNotImplemented()
{
	return "502 Command not implemented\r\n";
}

inline std::string closing()
{
	return "221 Bye\r\n";
}

} // namespace SmtpResponse
