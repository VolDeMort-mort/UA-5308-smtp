#pragma once

#include <fstream>
#include <shared_mutex>
#include <string>

#include "ILoggerStrategy.h"
#include "IReadable.h"

class FileStrategy : public ILoggerStrategy, public IReadable
{
public:
	FileStrategy(const std::string& path, LogLevel defaultLevel);

	~FileStrategy() override;

	void Flush() override;
	std::string SpecificLog(LogLevel lvl, const std::string& msg) override;
	std::vector<std::string> Read(size_t limit) override;
	std::vector<std::string> Search(LogLevel lvl, size_t limit, int read_n) override;
private:
	std::ofstream m_file;
	std::string m_current_path;
	LogLevel m_default_log_level;
	mutable std::shared_mutex m_shared_mutex;

	bool OpenFile();
	bool IsValid() override; // return true: if file is open

	void Write(const std::string& message) override;
};