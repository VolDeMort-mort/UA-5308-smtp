#pragma once

#include <fstream>
#include <mutex>
#include <string>

#include "ILoggerStrategy.h"
#include "IReadable.h"

/**
 * @class FileStrategy class (Strategy pattern)
 * @extends ILoggerStrategy, IReadable
 * @brief Strategy of writing logs to file
 */
class FileStrategy : public ILoggerStrategy, public IReadable
{
public:
	/**
	 * @brief FileStrategy ctor
	 * @param level Level of the logs(PROD, DEBUG, TRACE)
	 * @details open log file & set level
	 */
	FileStrategy(LogLevel defaultLevel);

	/**
	 * @brief FileStrategy dtor
	 * @details close log file
	 */
	~FileStrategy() override;

	/**
	 * @brief From ILoggerStrategy::Flush
	 */
	void Flush() override;
	bool IsValid() override; // return true, provide the file is open without fail, otherwise false

	/**
	 * @brief Writes a log message
	 * @param message The formatted string of the log
	 * @details writes log to a file, provide the file is open & valid
	 * @return True if writing succesfull
	 */
	bool Write(const std::string& message) override;

	/**
	 * @brief Formatting msg to a log message
	 * @param lvl Checking if current level < lvl
	 * @param msg Message which will be formatted
	 * @return Formatted or empty message
	 */
	std::string SpecificLog(LogLevel lvl, const std::string& msg) override;

	/**
	 * @brief From IReadable::Read
	 */
	std::vector<std::string> Read(size_t limit) override;

	/**
	 * @brief From IReadable::Search
	 */
	std::vector<std::string> Search(LogLevel lvl, size_t limit, int read_n) override;
	void Rotate(); // rotate file to new one, when current_size > MAX_FILE_SIZE

private:
	int m_current_file_size;
	std::ofstream m_file;
	std::string m_current_path;
	std::string m_old_path;
	LogLevel m_default_log_level;

	bool OpenFile();				 // open file & check for valid
	bool CanWrite(int message_size); // check current_size + msg_size <= MAX_FILE_SIZE
};