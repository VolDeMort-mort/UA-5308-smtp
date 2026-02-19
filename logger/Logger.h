#pragma once

#include <string>
#include <memory>

#include "ILoggerStrategy.h"

enum StrategyType
{
	File,
	Console
};

class Logger
{
public:
	Logger(StrategyType type, LogLevel level, const std::string& path);

	~Logger() = default;

	void set_strategy(StrategyType type);
	void set_default_level(LogLevel lvl);

	void Log(LogLevel level, const std::string& message);
	void Read();
	void Search();

private:
	std::unique_ptr<ILoggerStrategy> m_strategy;
	LogLevel m_default_level = LogLevel::PROD;
	std::string m_path;
};
