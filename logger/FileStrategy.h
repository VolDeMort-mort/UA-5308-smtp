#pragma once

#include <fstream>
#include <shared_mutex>
#include <string>

#include "ILoggerStrategy.h"
#include "IReadable.h"

const unsigned int MAX_FILE_SIZE = 10 * 1024 * 1024; //10mb || ~260k logs
const std::string FILE_PATH      = "log.txt";
const std::string OLD_FILE_PATH  = "old_log.txt";

class FileStrategy : public ILoggerStrategy, public IReadable
{
public:
	FileStrategy(LogLevel defaultLevel);
	~FileStrategy() override;
	void Flush() override;
	bool IsValid() override; //return true, if file is open without fail
	bool Write(const std::string& message) override;
	std::string SpecificLog(LogLevel lvl, const std::string& msg) override;
	std::vector<std::string> Read(size_t limit) override;
	std::vector<std::string> Search(LogLevel lvl, size_t limit, int read_n) override;
	void Rotate(); //rotate file to new one, when current_size > MAX_FILE_SIZE

private:
	int m_current_file_size;
	std::ofstream m_file;
	std::string m_current_path;
	std::string m_old_path;
	LogLevel m_default_log_level;
	mutable std::shared_mutex m_shared_mutex;

	bool OpenFile(); //open file & check for valid
	bool CanWrite(int message_size); //check current_size + msg_size <= MAX_FILE_SIZE
};