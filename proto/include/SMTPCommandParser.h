#pragma once

#include "SMTP_Types.h"

class SMTPCommandParser
{
public:
	static void ParseLine(std::string& line, ClientCommand& command);
};
