#pragma once

#include <string>

#include "ImapCommand.hpp"

class ImapParser
{
public:
	static ImapCommand Parse(const std::string& line);
};
