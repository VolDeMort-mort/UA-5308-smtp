#include "Logger.h"

Logger::Logger(std::unique_ptr<ILoggerStrategy> strategy, LogLevel level)
    : m_default_level(level)
{
    set_strategy(std::move(strategy));
}

void Logger::set_strategy(std::unique_ptr<ILoggerStrategy> strategy)
{
    m_strategy = std::move(strategy);
}

void Logger::Log(LogLevel lvl, const std::string& msg)
{
    if (!m_strategy) return;
    m_strategy->SpecificLog(lvl, msg);
}

std::vector<std::string> Logger::Read(size_t limit)
{
    if (!m_strategy) return {};
    return m_strategy->Read(limit);
}

std::vector<std::string> Logger::Search(LogLevel lvl, size_t limit, int readN)
{
    if (!m_strategy) return {};
    return m_strategy->Search(lvl, limit, readN);
}