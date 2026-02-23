#include "Logger.h"
#include "FileStrategy.h"
#include "ConsoleStrategy.h"

void Logger::set_strategy(StorageType type)
{
    if (type == File)
        m_strategy = std::make_unique<FileStrategy>(m_path,m_defaultLevel);
    else if(type == Console)
        m_strategy = std::make_unique<ConsoleStrategy>();
}

void Logger::set_level(LogLevel lvl)
{
    m_defaultLevel = lvl;
}

