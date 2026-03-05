#pragma once

#include "SMTP_Types.h"

class SMTPResponseParser
{
public:
	static void ParseLine(const std::vector<std::string>& line, ServerResponse& command);
	static bool TryParseCode(const std::string& line, int& out_code);
	static bool IsFinalSep(const std::string& line);
};
