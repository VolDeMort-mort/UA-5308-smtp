#pragma once
#include <string>

namespace ImapResponse
{

inline std::string ok(const std::string& tag, const std::string& message = "")
{
	return tag + " OK " + message + "\r\n";
}

inline std::string no(const std::string& tag, const std::string& message = "")
{
	return tag + " NO " + message + "\r\n";
}

inline std::string bad(const std::string& tag, const std::string& message = "")
{
	return tag + " BAD " + message + "\r\n";
}

inline std::string untagged(const std::string& message)
{
	return "* " + message + "\r\n";
}

inline std::string capability()
{
	return "* CAPABILITY IMAP4rev1 LOGIN LOGOUT SELECT LIST LSUB STATUS FETCH STORE CREATE DELETE RENAME IDLE MOVE EXPUNGE UID\r\n";
}

inline std::string flags(const std::string& flagList = "(\\Seen \\Answered \\Flagged \\Draft \\Recent)")
{
	return "* FLAGS " + flagList + "\r\n";
}

inline std::string exists(size_t count)
{
	return "* " + std::to_string(count) + " EXISTS\r\n";
}

inline std::string recent(size_t count)
{
	return "* " + std::to_string(count) + " RECENT\r\n";
}

inline std::string list(char separator, const std::string& mailbox, const std::string& attributes = "")
{
	return "* LIST (\\" + attributes + ") \"" + separator + "\" " + mailbox + "\r\n";
}

inline std::string status(const std::string& mailbox, size_t messages, size_t recent, size_t unseen)
{
	return "* STATUS " + mailbox + " (MESSAGES " + std::to_string(messages) + " RECENT " + std::to_string(recent) + " UNSEEN " + std::to_string(unseen) + ")\r\n";
}

inline std::string fetch(size_t msgNum, const std::string& data)
{
	return "* " + std::to_string(msgNum) + " FETCH " + data + "\r\n";
}

} // namespace ImapResponse
