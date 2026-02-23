#pragma once

#include <string>
#include <vector>

enum LogLevel
{
	NONE,
	PROD,
	DEBUG,
	TRACE
};

class ILoggerStrategy 
{
public:
	virtual ~ILoggerStrategy() = default;
	virtual void SpecificLog(LogLevel lvl, const std::string& msg) = 0;
	virtual std::vector<std::string> Read(size_t limit) = 0;
	virtual std::vector<std::string> Search(LogLevel lvl, size_t limit, int readN) = 0;

private:
	virtual void Write(const std::string& message) = 0;
};