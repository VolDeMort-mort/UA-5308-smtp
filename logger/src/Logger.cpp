#include "Logger.h"
#include "ConsoleStrategy.h"
#include "FileStrategy.h"
#include <iostream>
#include <thread>

Logger::Logger(std::shared_ptr<ILoggerStrategy> strategy)
{
	set_strategy(std::move(strategy));
	m_running_flag = true;
	m_flush = false;
	m_work_thread = std::thread(&Logger::WorkQueue, this);
}

void Logger::set_strategy(std::shared_ptr<ILoggerStrategy> strategy)
{
	std::lock_guard<std::mutex> lock(m_strategy_mtx);
	if (!strategy) return;
	if (strategy->IsValid())
		m_strategy = std::move(strategy);
	else
		m_strategy = std::make_shared<ConsoleStrategy>();
}

void Logger::PushToQueue(const std::string& message)
{
	std::lock_guard<std::mutex> lock(m_queue_mtx);
	m_queue.push(message);
	if (m_queue.size() >= 50) 
		m_cv.notify_one();
};

void Logger::WorkQueue()
{
	std::queue<std::string> local_queue;
	while (true)
	{
		{
			std::unique_lock<std::mutex> lock(m_queue_mtx);
			// wait for batch size, flush request from Read(), or timeout
			m_cv.wait_for(lock, std::chrono::seconds(2),[this] { 
				return m_queue.size() >= 50 || m_flush || !m_running_flag; });

			// trigger on Read(), when logs are flushed
			if (m_flush && m_queue.empty())
			{
				m_cv.notify_all(); // notify Read() that logs are pushed

				// wait for !m_flush from Read(), logs in queue or shutdown, also prevent loop
				m_cv.wait(lock, [this] { return !m_flush || !m_queue.empty() || !m_running_flag.load(); });
			}
			if (m_queue.empty() && m_running_flag) 
				continue;
			if (m_queue.empty() && !m_running_flag) // leave thread when queue is empty & logger is off
				break;

			std::swap(local_queue, m_queue);
		}

		if (!local_queue.empty())
		{
			std::shared_ptr<ILoggerStrategy> local_strategy;
			{
				std::lock_guard<std::mutex> strategy_lock(m_strategy_mtx);
				local_strategy = m_strategy;
			}
			if (!local_strategy) continue;

			if (!local_strategy->IsValid()) // if path is invalid,logs can`t be pushed into file
			{
				std::lock_guard<std::mutex> strategy_lock(m_strategy_mtx);
				std::cerr << "Log strategy failure.\n";
				m_strategy = std::make_shared<ConsoleStrategy>(); // set console output to prevent logs leaks
				local_strategy = m_strategy;
			}
			while (!local_queue.empty())
			{
				if (!local_strategy->Write(local_queue.front()))
				{
					std::lock_guard<std::mutex> strategy_lock(m_strategy_mtx);
					std::cerr << "Log strategy failure.\n";
					m_strategy = std::make_shared<ConsoleStrategy>();
					local_strategy = m_strategy;
					continue;
				}
				local_queue.pop();
			}
			local_strategy->Flush();
		}
	}
};

void Logger::Log(LogLevel lvl, const std::string& msg)
{
	std::shared_ptr<ILoggerStrategy> local_strategy;
	{
		std::lock_guard<std::mutex> strategy_lock(m_strategy_mtx);
		local_strategy = m_strategy;
	}
	if (!local_strategy) return;

	std::string message = local_strategy->SpecificLog(lvl, msg);

	this->PushToQueue(message);
}

std::vector<std::string> Logger::Read(size_t limit)
{
	{
		std::unique_lock<std::mutex> lock(m_queue_mtx);
		m_flush = true;
		m_cv.notify_all(); // notify thread for flush

		// wait empty queue or shutdown to avoid crash
		m_cv.wait(lock, [this] { return m_queue.empty() || !m_running_flag.load(); });
		m_flush = false;
		m_cv.notify_all(); // notify thread for !flush
	}
	std::shared_ptr<ILoggerStrategy> local_strategy;
	{
		std::lock_guard<std::mutex> strategy_lock(m_strategy_mtx);
		local_strategy = m_strategy;
	}

	if (!local_strategy) return {};

	// safer casting for shared_ptr than dynamic_cast
	auto readable = std::dynamic_pointer_cast<IReadable>(local_strategy);
	if (!readable) return {};
	return readable->Read(limit);
}

std::vector<std::string> Logger::Search(LogLevel lvl, size_t limit, int read_n)
{
	std::shared_ptr<ILoggerStrategy> local_strategy;
	{
		std::lock_guard<std::mutex> strategy_lock(m_strategy_mtx);
		local_strategy = m_strategy;
	}

	if (!local_strategy) return {};

	auto searchable = std::dynamic_pointer_cast<IReadable>(local_strategy);

	if (!searchable) return {};
	return searchable->Search(lvl, limit, read_n);
}
Logger::~Logger()
{
	m_running_flag = false;
	m_cv.notify_all();
	if (m_work_thread.joinable()) m_work_thread.join();
}
