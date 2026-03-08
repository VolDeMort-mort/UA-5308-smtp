#pragma once

#include <string>
#include <vector>

#include "ILoggerStrategy.h"
/**
 * @class IReadable
 * @brief Provides Read & Search methods for FileStrategy
 */
class IReadable
{
public:
	/**
	 * @brief Reading the count of logs
	 * @param limit Maximum count of logs which will be read
	 * @return Vector of strings from storage(only for file)
	 */
	virtual std::vector<std::string> Read(size_t limit) = 0;

	/**
	 * @brief Search logs in file by lvl param
	 * @param level Filters logs by level
	 * @param limit Max count of finding logs for return
	 * @param read_n Count of logs in which will be searching
	 * @return vector of strings from storage(only for file)
	 */
	virtual std::vector<std::string> Search(LogLevel lvl, size_t limit, int read_n) = 0;
	virtual ~IReadable() = default; // default virtual dtor
};