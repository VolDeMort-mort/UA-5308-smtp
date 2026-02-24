#pragma once

#include <string>
#include <memory>
#include <vector>

#include "ILoggerStrategy.h"
#include "IReadable.h"

class Logger
{
public:
    Logger(std::unique_ptr<ILoggerStrategy> strategy, LogLevel level);

    ~Logger() = default;

    void set_strategy(std::unique_ptr<ILoggerStrategy> strategy);

    void Log(LogLevel level, const std::string& message);
    std::vector<std::string> Read(size_t limit);
    std::vector<std::string> Search(LogLevel lvl, size_t limit, int read_n);

private:
    std::unique_ptr<ILoggerStrategy> m_strategy;
    LogLevel m_default_level = LogLevel::PROD;
};