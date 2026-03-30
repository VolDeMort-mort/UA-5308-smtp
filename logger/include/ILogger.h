#pragma once

#include <string>

#include "ILoggerStrategy.h"

/**
 * @class ILogger
 * @brief Interface for log operation
 */
class ILogger
{
public:
	virtual ~ILogger() = default;

	/**
	 * @brief Add raw message into queue
	 * @param level Level of the logs(PROD, DEBUG, TRACE)
	 * @param message Raw text of the log
	 */
	virtual void Log(LogLevel level, const std::string& message) = 0;

	virtual void set_level(LogLevel level) = 0; // runtime level switching

protected:
	/**
	 * @brief Set new strategy in runtime
	 * @param strategy New strategy for logging. Logs are saving with switching
	 */
	virtual void set_strategy(std::shared_ptr<ILoggerStrategy> strategy) = 0;
};