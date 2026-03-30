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
	ILoggerStrategy(LogLevel lvl) : m_default_log_level(lvl) {};

	virtual ~ILoggerStrategy() = default;

	/**
	 * @brief Formats a log message for certain strategy
	 * @param lvl The level of the log
	 * @param msg A raw message
	 * @return A formatted message
	 */
	virtual std::string SpecificLog(LogLevel lvl, const std::string& msg) = 0;

	virtual bool IsValid() = 0;
	virtual bool Write(const std::string& message) = 0;
	virtual void Flush() = 0;
	virtual std::string get_name() const = 0;
	virtual std::string get_last_error() const { return m_last_error; }
	LogLevel get_current_level() const { return m_default_log_level; }
	void set_log_level(LogLevel lvl) { m_default_log_level = lvl; };

protected:
	LogLevel m_default_log_level;
	std::string m_last_error;
};
