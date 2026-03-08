#pragma once
#include "../../logger/ILoggerStrategy.h"

class MockLogger : public ILoggerStrategy
{
public:
	void SpecificLog(LogLevel, const std::string&) override {}

private:
	void Write(const std::string&) override {}
};
