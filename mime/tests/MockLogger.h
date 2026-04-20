#pragma once
#include "ILoggerStrategy.h"

class MockStrategy : public ILoggerStrategy
{
public:
	MockStrategy(LogLevel lvl = PROD) : ILoggerStrategy(lvl) {}

	std::string SpecificLog(LogLevel, const std::string& msg) override { return msg; }
	bool        IsValid() override { return true; }
	bool        Write(const std::string&) override { return true; }
	void        Flush() override {}
	std::string get_name() const override { return "Mock"; }
};
