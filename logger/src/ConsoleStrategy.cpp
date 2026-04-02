#include <iostream>
#include <cstring>
#include <algorithm>
#include <thread>

#include "ConsoleStrategy.h"
#include "LoggerConfig.h"

ConsoleStrategy::ConsoleStrategy(LogLevel defaultLevel) :
	ILoggerStrategy(defaultLevel), m_current_level(NONE) {}

std::string ConsoleStrategy::get_name() const
{
	return "Console";
}
bool ConsoleStrategy::Write(const std::string& message)
{
	std::string color;

	switch (m_current_level)
	{
	case INFO:
		color = ISXLoggerConfig::Color::Info;
		break;
	case ERROR:
		color = ISXLoggerConfig::Color::Error;
		break;
	case PROD:
		color = ISXLoggerConfig::Color::Prod;
		break;
	case DEBUG:
		color = ISXLoggerConfig::Color::Debug;
		break;
	case TRACE:
		color = ISXLoggerConfig::Color::Trace;
		break;
	default:
		color = ISXLoggerConfig::Color::Reset;
		break;
	}

	std::cout << color + message + ISXLoggerConfig::Color::Reset << "\n";

	return true;
}
std::string ConsoleStrategy::SpecificLog(LogLevel lvl, const std::string& msg)
{
	if (lvl > m_default_log_level) return {};

	auto now = std::chrono::system_clock::now();
	auto sec = std::chrono::system_clock::to_time_t(now);

	std::tm tm{}; // struct for date and time(from <chrono>)

#ifdef _WIN32 // on win and unix sys we need different function calls
	localtime_s(&tm, &sec);
#else
	localtime_r(&sec, &tm);
#endif

	const char* levelStr = "UNKNOWN";
	switch (lvl)
	{
	case LogLevel::NONE:
		levelStr = "NONE";
		break;
	case LogLevel::INFO:
		levelStr = "INFO";
		break;
	case LogLevel::ERROR:
		levelStr = "ERROR";
		break;
	case LogLevel::PROD:
		levelStr = "PROD";
		break;
	case LogLevel::DEBUG:
		levelStr = "DEBUG";
		break;
	case LogLevel::TRACE:
		levelStr = "TRACE";
		break;
	}

	auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());

	char buffer[512];

	int headerLen = std::snprintf(
		buffer, sizeof(buffer), "[%04d-%02d-%02d %02d:%02d:%02d] [%s] [tid:%lu] ",
		tm.tm_year + 1900, // tm struc counts ammount of years passed since 1900(thats why we need +1900 to get 2026)
		tm.tm_mon + 1,	   // months are indexes, here we convert for normal view
		tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, levelStr, tid);

	if (headerLen < 0) return {};

	size_t remaining = sizeof(buffer) - static_cast<size_t>(headerLen);
	size_t copyLen = std::min(msg.size(), remaining - 2);

	std::memcpy(buffer + headerLen, msg.data(), copyLen);

	size_t totalLen = headerLen + copyLen;
	buffer[totalLen++] = '\n';
	buffer[totalLen] = '\0';

	m_current_level = lvl;

	return std::string(buffer, totalLen);
}
void ConsoleStrategy::Flush()
{
	std::cout.flush();
}
bool ConsoleStrategy::IsValid()
{
	return true;
}