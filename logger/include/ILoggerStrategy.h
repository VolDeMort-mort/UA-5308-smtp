#pragma once
#include <string>

/**
 * @enum LogLevel
 * @brief Enum class for log levels
 */
enum LogLevel
{
	NONE,
	PROD,
	DEBUG,
	TRACE
};

/**
 * @class ILoggerStrategy
 * @brief Interface for logging strategies (Strategy pattern)
 * (File, Console)
 */
class ILoggerStrategy
{
public:
	virtual ~ILoggerStrategy() = default; // default virtual dtor

	/**
	 * @brief Formats a log message for certain strategy
	 * @param lvl The level of the log
	 * @param msg A raw message
	 * @return A formatted message
	 */
	virtual std::string SpecificLog(LogLevel lvl, const std::string& msg) = 0;

	virtual bool IsValid() = 0;	// checks if the output storage is ready
	virtual bool Write(const std::string& message) = 0; // writes to storage
	virtual void Flush() = 0; // flushes log to a storage
};
