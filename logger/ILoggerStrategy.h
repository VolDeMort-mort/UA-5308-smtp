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
	virtual void SpecificLog(LogLevel lvl, const std::string& msg) = 0;

private:
	virtual void Write(const std::string& message) = 0;
};