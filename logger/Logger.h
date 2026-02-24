#pragma once
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "ILoggerStrategy.h"
#include "IReadable.h"

const std::string FILE_PATH = "log.txt";

class Logger
{
public:
	Logger(std::unique_ptr<ILoggerStrategy> strategy);

	~Logger();

	void set_strategy(std::unique_ptr<ILoggerStrategy> strategy);

	void Log(LogLevel level, const std::string& message);
	std::vector<std::string> Read(size_t limit);
	std::vector<std::string> Search(LogLevel lvl, size_t limit, int read_n);

private:
	std::unique_ptr<ILoggerStrategy> m_strategy;
	LogLevel m_defaultLevel = LogLevel::PROD;

	std::mutex m_queue_mtx;
	std::mutex m_strategy_mtx;
	std::queue<std::string> m_queue;
	std::condition_variable m_cv;
	std::thread m_work_thread;
	std::atomic<bool> m_running_flag;

	void PushToQueue(const std::string& message);
	void WorkQueue();
};
