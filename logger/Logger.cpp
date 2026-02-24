#include "Logger.h"

#include <iostream>

#include "ConsoleStrategy.h"
#include "FileStrategy.h"

Logger::Logger(std::unique_ptr<ILoggerStrategy> strategy)
{
	m_running_flag = true;
	set_strategy(std::move(strategy));
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
	if (m_queue.size() >= 50) m_cv.notify_one();
};

void Logger::WorkQueue()
{
	std::queue<std::string> local_queue;
	while (m_running_flag)
	{
		{
			std::unique_lock<std::mutex> lock(m_queue_mtx);
			m_cv.wait_for(lock, std::chrono::seconds(2), [this] { return m_queue.size() >= 50; });
			if (m_queue.empty() && m_running_flag) continue;
			if (m_queue.empty() && !m_running_flag) break;

			std::swap(local_queue, m_queue);
		}

		if (!local_queue.empty())
		{
			if (!m_strategy->IsValid()) // if path is invalid,logs can`t be pushed into file
			{
				std::cerr << "Can`t open log file: " << FILE_PATH << "\n";
				m_strategy = std::make_unique<ConsoleStrategy>();
			}
			while (!local_queue.empty())
			{
				m_strategy->Write(local_queue.front());
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
