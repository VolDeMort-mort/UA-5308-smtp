#pragma once
#include "ImapCommand.hpp"
#include <string>

class ImapParser
{
public:
	static ImapCommand parse(const std::string& line);
};
