#pragma once

#include <string>

#include "ILoggerStrategy.h"

/**
 * @class ILogger
 * @brief Interface for log operation
 * @details Logging interface for different modules (IMAP integration for now)
 */
class ILogger
{
public:
	virtual ~ILogger() = default; // default virtual dtor

	/**
	 * @brief Logs a message with a log level
	 * @param level Level of the message
	 * @param message Raw text of the log
	 */
	virtual void Log(LogLevel level, const std::string& message) = 0;
};