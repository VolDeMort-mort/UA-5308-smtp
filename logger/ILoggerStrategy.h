#pragma once
#include <string>

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
	virtual std::string SpecificLog(LogLevel lvl, const std::string& msg) = 0;
	virtual bool IsValid() = 0;
	virtual bool Write(const std::string& message) = 0;
	virtual void Flush() = 0;
};
