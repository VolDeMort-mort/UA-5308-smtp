#pragma once
#include "SmtpCommand.hpp"

class SmtpParser {
public:
    static SmtpCommand parse(const std::string& line);
};

