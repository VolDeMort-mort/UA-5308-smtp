#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include "ILoggerStrategy.h"
#include "ILogger.h"
#include "Queue.h"
#include "LoggerConfig.h"

class LockFreeLogger : public ILogger
{
public:
	LockFreeLogger(std::shared_ptr<ILoggerStrategy> strategy);
	~LockFreeLogger();
	void Log(LogLevel level, const std::string& message) override;
	void set_strategy(std::shared_ptr<ILoggerStrategy> strategy);
private:
	std::shared_ptr<ILoggerStrategy> m_strategy;
	Queue<std::pair<LogLevel, std::string>, ISXLoggerConfig::LogsCapacity> m_ringBuffer;
	std::thread m_work_thread;
	void WorkQueue();
	std::atomic<bool> m_stop_flag;
};