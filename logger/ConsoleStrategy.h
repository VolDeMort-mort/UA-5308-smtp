#pragma once
#include "ILoggerStrategy.h"

class ConsoleStrategy : public ILoggerStrategy {
public:
	ConsoleStrategy() {};
	void SpecificLog(LogLevel lvl, const std::string& msg) override;

	~ConsoleStrategy() {};

private:
	void Write(const std::string& message) override;
};

