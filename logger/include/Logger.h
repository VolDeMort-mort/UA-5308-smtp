#pragma once
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "ILogger.h"
#include "ILoggerStrategy.h"
#include "IReadable.h"

/**
 * @class Logger
 * @brief Asynchronous logger with batching mechanism
 * @details Using Strategy pattern for runtime switching storages
 * logs are filling in queue and writing in storage in batches when reach limit or timeout
 */

class Logger : public ILogger
{
public:
	/**
	 * @brief Logger ctor
	 * @param strategy pointer to writing strategy(File,Console)
	 */
	Logger(std::shared_ptr<ILoggerStrategy> strategy);

	/**
	 * @brief Logger dtor
	 * @details guarantees write all logs to storage from queue before leaving
	 */
	~Logger();

	/**
	 * @brief From ILogger::set_strategy
	 */
	void set_strategy(std::shared_ptr<ILoggerStrategy> strategy) override;

	/**
	 * @brief From ILogger::set_level
	 */
	void set_level(LogLevel level) override;

	/**
	 * @brief From ILogger::Log
	 */
	void Log(LogLevel level, const std::string& message) override;

	/**
	 * @brief Thread-safe logs reading from strategy
	 * @details Force flushing to the storage before reading
	 * @param limit Maximum number of logs
	 * @return Vector of log strings or empty
	 */
	std::vector<std::string> Read(size_t limit);

	/**
	 * @brief Search for logs from file
	 * @details Force flushing to the storage before searching
	 * @param lvl Log level to filter by
	 * @param limit Maximum number of logs
	 * @param read_n Number of logs to scan from the end of the storage
	 * @return Vector of filtered log strings or empty
	 */
	std::vector<std::string> Search(LogLevel lvl, size_t limit, int read_n);

private:
	std::shared_ptr<ILoggerStrategy> m_strategy; // pointer to abstract class

	std::mutex m_queue_mtx;								  // mutex for queue & Read()
	std::mutex m_strategy_mtx;							  // mutex for safety runtime strategy switching
	std::queue<std::pair<LogLevel, std::string>> m_queue; // queue with logs
	std::condition_variable m_cv;						  // cv for queue and Read()
	std::thread m_work_thread;							  // thread for queue
	std::atomic<bool> m_running_flag;					  // flag for queue life cycle(false - break cycle)
	std::atomic<bool> m_flush;							  // flag for Read() to force flush logs
	std::atomic<bool> m_is_flushed;						  // flag that logs are flushed

	/**
	 * @brief Async thread with local queue
	 * @details Implementing waiting logic for batch or timeout
	 * before writing, formatting the raw message from queue
	 */
	void WorkQueue();

	/**
	 * @brief Flushing logs
	 * @details Read & Search methods waiting for notify from WorkQueue
	 * to read or search actual logs
	 */
	void WaitFlush();

	/**
	 * @brief Report logger info
	 * @details Writing to console a log info
	 * @param msg Message of the log
	 */
	void ReportSystemInfo(const std::string& msg);

	/**
	 * @brief Report logger error
	 * @details Writing to console a log error
	 * @param msg Message of the log
	 */
	void ReportError(const std::string& msg);
};
