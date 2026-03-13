#pragma once
#include "../../logger/ILoggerStrategy.h"

class MockStrategy : public ILoggerStrategy
{
public:
	std::string SpecificLog(LogLevel, const std::string& msg) override { return msg; }
	bool        IsValid() override { return true; }
	bool        Write(const std::string&) override { return true; }
	void        Flush() override {}
};
