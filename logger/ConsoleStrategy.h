#pragma once
#include "ILoggerStrategy.h"

class ConsoleStrategy : public ILoggerStrategy
{
private:
	void Write(const std::string& message) override;

public:
	ConsoleStrategy();
	std::string SpecificLog(LogLevel lvl, const std::string& msg) override;
	bool IsValid() override;
	void Flush() override;
	~ConsoleStrategy() {};
};



