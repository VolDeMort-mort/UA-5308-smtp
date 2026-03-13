#pragma once

#include <string>

#include "SmtpCommand.hpp"

class SmtpParser
{
public:
    static SmtpCommand Parse(const std::string& line);
};
