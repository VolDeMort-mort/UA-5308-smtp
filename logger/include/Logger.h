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
	Logger(std::unique_ptr<ILoggerStrategy> strategy);

	/**
	 * @brief Logger dtor
	 * @details guarantees write all logs to storage from queue before leaving
	 */
	~Logger();

	/**
	 * @brief Set new strategy in runtime
	 * @param strategy New strategy for logging. Logs are saving with switching
	 */
	void set_strategy(std::unique_ptr<ILoggerStrategy> strategy);

	/**
	 * @brief Formatting message and push it into PushToQueue()
	 * @param level Level of the logs(PROD, DEBUG, TRACE)
	 * @param message Raw text of the log
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
	 * @details Force flushing to the storage before reading
	 * @param lvl Log level to filter by
	 * @param limit Maximum number of logs
	 * @param read_n Number of logs to scan from the end of the storage
	 * @return Vector of filtered log strings or empty
	 */
	std::vector<std::string> Search(LogLevel lvl, size_t limit, int read_n);

private:
	std::unique_ptr<ILoggerStrategy> m_strategy; // pointer to abstract class
	LogLevel m_defaultLevel = LogLevel::PROD;	 // default lvl for logs

	std::mutex m_queue_mtx;			  // mutex for queue & Read()
	std::mutex m_strategy_mtx;		  // mutex for safety runtime strategy switching
	std::queue<std::string> m_queue;  // queue with logs
	std::condition_variable m_cv;	  // cv for queue and Read()
	std::thread m_work_thread;		  // thread for queue
	std::atomic<bool> m_running_flag; // flag for queue life cycle(false - break cycle)
	std::atomic<bool> m_flush;		  // flag for Read() to force flush logs
	std::atomic<bool> m_flush_done;

	/**
	 * @brief Add logs into m_queue
	 * @param message The formatted string of the log
	 */
	void PushToQueue(const std::string& message);

	/**
	 * @brief Async thread with local queue
	 * @details Implementing waiting logic for batch or timeout
	 */
	void WorkQueue();
};
