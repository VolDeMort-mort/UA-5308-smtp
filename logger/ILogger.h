#pragma once

#include <string>

#include "ILoggerStrategy.h"

class ILogger
{
public:
	virtual ~ILogger() = default;
	virtual void Log(LogLevel level, const std::string& message) = 0;
};