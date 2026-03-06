#pragma once
#include "ILoggerStrategy.h"

/**
 * @class ConsoleStrategy class (Strategy pattern)
 * @extends ILoggerStrategy
 * @brief Strategy of writing logs to file
 */
class ConsoleStrategy : public ILoggerStrategy
{
public:
	ConsoleStrategy();
	~ConsoleStrategy() {};

	/**
	 * @brief Formatting msg to a log message
	 * @param lvl The level of the log
	 * @param msg Message which will be formatted
	 * @return Formatted or empty message
	 */
	std::string SpecificLog(LogLevel lvl, const std::string& msg) override;

	bool IsValid() override; // Always true for console

	/**
	 * @brief Writes a log message
	 * @param message The formatted string of the log
	 * @details writes log to a console
	 * @return True if writing succesfull
	 */
	bool Write(const std::string& message) override;

	/**
	 * @brief From ILoggerStrategy::Flush
	 */
	void Flush() override;
};
