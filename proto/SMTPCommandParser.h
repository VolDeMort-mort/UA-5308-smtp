#pragma once
#include "SMTP_Types.h"


class SMTPCommandParser
{
public:
	static void parseLine(std::string& line, ClientCommand& command);
};

