#pragma once

#include <vector>
#include <string>

#include "ILoggerStrategy.h"

class IReadable
{
public:
    virtual std::vector<std::string> Read(size_t limit) = 0;
    virtual std::vector<std::string> Search(LogLevel lvl, size_t limit, int read_n) = 0;
    virtual ~IReadable() = default;
};