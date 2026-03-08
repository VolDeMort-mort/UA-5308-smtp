#pragma once
#include "../../logger/ILoggerStrategy.h"

class MockLogger : public ILoggerStrategy
{
public:
	std::string SpecificLog(LogLevel, const std::string&) override { return ""; }
	bool IsValid() override { return true; }
	bool Write(const std::string&) override { return true; }
	void Flush() override {}
};
