#pragma once

#include <string>

namespace ImapResponse
{

inline std::string Ok(const std::string& tag, const std::string& message = "")
{
	return tag + " OK " + message + "\r\n";
}

inline std::string No(const std::string& tag, const std::string& message = "")
{
	return tag + " NO " + message + "\r\n";
}

inline std::string Bad(const std::string& tag, const std::string& message = "")
{
	return tag + " BAD " + message + "\r\n";
}

inline std::string Untagged(const std::string& message)
{
	return "* " + message + "\r\n";
}

inline std::string Capability()
{
	return "* CAPABILITY IMAP4rev1 LOGIN LOGOUT SELECT LIST LSUB STATUS FETCH STORE CREATE DELETE RENAME IDLE MOVE "
		   "EXPUNGE UID\r\n";
}

inline std::string Flags(const std::string& flagList = "(\\Seen \\Answered \\Flagged \\Draft \\Recent)")
{
	return "* FLAGS " + flagList + "\r\n";
}

inline std::string Exists(size_t count)
{
	return "* " + std::to_string(count) + " EXISTS\r\n";
}

inline std::string Recent(size_t count)
{
	return "* " + std::to_string(count) + " RECENT\r\n";
}

inline std::string List(char separator, const std::string& mailbox, const std::string& attributes = "")
{
	return "* LIST (\\" + attributes + ") \"" + separator + "\" " + mailbox + "\r\n";
}

inline std::string Status(const std::string& mailbox, const std::string& statusData)
{
	return "* STATUS " + mailbox + " (" + statusData + ")\r\n";
}

inline std::string Fetch(size_t msgNum, const std::string& data)
{
	return "* " + std::to_string(msgNum) + " FETCH " + data + "\r\n";
}

} // namespace ImapResponse
