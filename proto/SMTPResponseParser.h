#pragma once
#include "SMTP_Types.h"

class SMTPResponseParser
{
public:
	static void parseLine(const std::vector<std::string>& line, ServerResponse& command);
	static bool try_parse_code(const std::string& line, int& out_code);
	static bool is_final_sep(const std::string& line);
};

