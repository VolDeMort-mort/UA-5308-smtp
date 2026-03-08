#pragma once
#include "ILoggerStrategy.h"

class ConsoleStrategy : public ILoggerStrategy
{
public:
	ConsoleStrategy();
	~ConsoleStrategy() {};
	std::string SpecificLog(LogLevel lvl, const std::string& msg) override;
	bool IsValid() override;
	bool Write(const std::string& message) override;
	void Flush() override;
};





