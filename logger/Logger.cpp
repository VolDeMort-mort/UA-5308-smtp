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
    auto* readable = dynamic_cast<IReadable*>(m_strategy.get());

    if(!readable) return {};

    return readable->Read(limit);
}

std::vector<std::string> Logger::Search(LogLevel lvl, size_t limit, int read_n)
{
    auto* searchable = dynamic_cast<IReadable*>(m_strategy.get());

    if(!searchable) return {};

    return searchable->Search(lvl, limit, read_n);
}