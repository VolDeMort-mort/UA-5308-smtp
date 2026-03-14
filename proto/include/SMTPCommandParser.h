#pragma once

#include "SMTP_Types.h"

class SMTPCommandParser
{
public:
	static ClientCommand ParseLine(std::string_view line);

	static ClientCommand ParseData(std::string_view data);
};
