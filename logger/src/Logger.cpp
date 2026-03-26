#include <iostream>
#include <thread>

#include "Logger.h"
#include "FileStrategy.h"
#include "ConsoleStrategy.h"
#include "LoggerConfig.h"

Logger::Logger(std::shared_ptr<ILoggerStrategy> strategy)
{
	ReportSystemInfo("Logger start working");
	set_strategy(std::move(strategy));
	m_running_flag = true;
	m_flush = false;
	m_is_flushed = false;
	m_work_thread = std::thread(&Logger::WorkQueue, this);
}

void Logger::set_strategy(std::shared_ptr<ILoggerStrategy> strategy)
{
	std::lock_guard<std::mutex> lock(m_strategy_mtx);
	if (!strategy) return;
	if (strategy->IsValid())
	{
		m_strategy = std::move(strategy);
		ReportSystemInfo(m_strategy->get_name() + " output is enabled");
	}
	else
	{
		std::string error = "Invalid strategy";
		if (strategy)
		{
			std::string error_message = strategy->get_last_error();
			if (!error_message.empty())
				error = error_message;
		}
		else
			error = "No strategy";

		ReportError("Failed to initialize strategy: " + error);

		LogLevel level = (m_strategy != nullptr) ? m_strategy->get_current_level() : LogLevel::PROD;

		m_strategy = std::make_shared<ConsoleStrategy>(level);

		ReportSystemInfo("Console output with log level " + std::to_string(level) + " is enabled");
	}
}

void Logger::WorkQueue()
{
	std::queue<std::pair<LogLevel, std::string>> local_queue;
	while (true)
	{
		{
			std::unique_lock<std::mutex> lock(m_queue_mtx);
			// wait for batch size, flush request from Read(), or timeout
			m_cv.wait_for(lock, ISXLoggerConfig::Timeout, [this] {
					return m_queue.size() >= ISXLoggerConfig::LogsForBatch || m_flush || !m_running_flag.load(); });

			// trigger on Read(), when queue empty
			if (m_flush && m_queue.empty())
			{
				m_is_flushed = true;
				m_cv.notify_all(); // notify Read() that logs are pushed

				// wait for !m_flush from Read(), logs in queue or shutdown, also prevent loop
				m_cv.wait(lock, [this] { return !m_flush || !m_queue.empty() || !m_running_flag.load(); });
			}
			if (m_queue.empty() && m_running_flag.load())
				continue;
			if (m_queue.empty() && !m_running_flag.load()) // leave thread when queue is empty & logger is off
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

			if (!local_strategy || !local_strategy->IsValid()) // if path is invalid,logs can`t be pushed into file
			{
				std::lock_guard<std::mutex> strategy_lock(m_strategy_mtx);
				ReportError("Log strategy failure");
				m_strategy = std::make_shared<ConsoleStrategy>(m_strategy->get_current_level()); // set console output to prevent logs leaks
				ReportSystemInfo("Console output is enabled");
				local_strategy = m_strategy;
			}
			while (!local_queue.empty())
			{
				std::string formatted_message =
					local_strategy->SpecificLog(local_queue.front().first, local_queue.front().second);

				if (formatted_message.empty())
				{
					local_queue.pop();
					continue;
				}

				if (!local_strategy->Write(formatted_message))
				{
					std::lock_guard<std::mutex> strategy_lock(m_strategy_mtx);
					ReportError("Log strategy writing failure. " + m_strategy->get_last_error());
					m_strategy = std::make_shared<ConsoleStrategy>(m_strategy->get_current_level());
					ReportSystemInfo("Console output is enabled");
					local_strategy = m_strategy;
					continue;
				}
				local_queue.pop();
			}
			local_strategy->Flush();

			std::unique_lock<std::mutex> lock(m_queue_mtx);
			if (m_flush && m_queue.empty())
			{
				m_is_flushed = true; // confirm flush
				m_cv.notify_all();
			}
		}
	}
};

void Logger::Log(LogLevel lvl, const std::string& msg)
{
	if (lvl == NONE) return;
	if (!m_running_flag.load()) return;
	std::lock_guard<std::mutex> lock(m_queue_mtx);
	m_queue.emplace(lvl, msg);
	if (m_queue.size() >= ISXLoggerConfig::LogsForBatch)
		m_cv.notify_one();
}

std::vector<std::string> Logger::Read(size_t limit)
{
	{
		std::unique_lock<std::mutex> lock(m_queue_mtx);
		m_flush = true;
		m_cv.notify_all(); // notify thread for flush

		// wait empty queue and flag or shutdown to avoid crash
		m_cv.wait(lock, [this] { return (m_queue.empty() && m_is_flushed) || !m_running_flag.load(); });
		m_flush = false;
		m_is_flushed = false;
		m_cv.notify_all(); // notify thread for !flush
	}
	std::shared_ptr<ILoggerStrategy> local_strategy;
	{
		std::lock_guard<std::mutex> strategy_lock(m_strategy_mtx);
		local_strategy = m_strategy;
	}

	if (!local_strategy) return {};

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
void Logger::ReportSystemInfo(const std::string& msg)
{
	std::cout << ISXLoggerConfig::Color::Info << "[LOGGER INFO] " << msg << ISXLoggerConfig::Color::Reset << std::endl;
}
void Logger::ReportError(const std::string& msg)
{
	std::cerr << ISXLoggerConfig::Color::Error << "[LOGGER ERROR] " << msg << ISXLoggerConfig::Color::Reset
			  << std::endl;
}
Logger::~Logger()
{
	m_running_flag = false;
	m_cv.notify_all();
	if (m_work_thread.joinable())
		m_work_thread.join();
	ReportSystemInfo("Logger finished working");
}
