#include "Logger.h"
#include <iostream>
#include <thread>
#include "ConsoleStrategy.h"
#include "FileStrategy.h"

Logger::Logger(std::unique_ptr<ILoggerStrategy> strategy)
{
	set_strategy(std::move(strategy));
	m_running_flag = true;
	m_flush = false;
	m_work_thread = std::thread(&Logger::WorkQueue, this);
}

void Logger::set_strategy(std::unique_ptr<ILoggerStrategy> strategy)
{
	if (!strategy) return;

	std::lock_guard<std::mutex> lock(m_strategy_mtx);
	if (strategy->IsValid())
		m_strategy = std::move(strategy);
	else
		m_strategy = std::make_unique<ConsoleStrategy>();
}

void Logger::PushToQueue(const std::string& message)
{
	{
		std::lock_guard<std::mutex> lock(m_queue_mtx);
		m_queue.push(message);
	}
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
			m_cv.wait_for(lock, std::chrono::seconds(2),
						  [this] { return m_queue.size() >= 50 || m_flush || !m_running_flag; });

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
			if (!m_strategy->IsValid()) // if path is invalid,logs can`t be pushed into file
			{
				std::cerr << "Log strategy failure.\n";
				m_strategy = std::make_unique<ConsoleStrategy>(); // set console output to prevent logs leaks
			}
			while (!local_queue.empty())
			{
				if (!m_strategy->Write(local_queue.front()))
				{
					std::cerr << "Log strategy failure.\n";
					m_strategy = std::make_unique<ConsoleStrategy>();
					continue;
				}
				local_queue.pop();
			}
		}
		m_strategy->Flush();
	}
};

void Logger::Log(LogLevel lvl, const std::string& msg)
{
	if (!m_strategy) return;

	std::string message = m_strategy->SpecificLog(lvl, msg);

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

	m_strategy->Flush();

	auto* readable = dynamic_cast<IReadable*>(m_strategy.get());

	if (!readable) return {};

	return readable->Read(limit);
}

std::vector<std::string> Logger::Search(LogLevel lvl, size_t limit, int read_n)
{
	auto* searchable = dynamic_cast<IReadable*>(m_strategy.get());

	if (!searchable) return {};

	return searchable->Search(lvl, limit, read_n);
}
Logger::~Logger()
{
	m_running_flag = false;
	m_cv.notify_all();
	if (m_work_thread.joinable()) m_work_thread.join();
}
