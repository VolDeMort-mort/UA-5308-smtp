#include <queue>
#include <memory>
#include <chrono>

#include "LockFreeLogger.h"
#include "FileStrategy.h"
#include "ConsoleStrategy.h"
#include "LoggerConfig.h"

LockFreeLogger::LockFreeLogger(std::shared_ptr<ILoggerStrategy> strategy) 
	: m_stop_flag(false)
{
	set_strategy(strategy);
	m_work_thread = std::thread(&LockFreeLogger::WorkQueue, this);
}
void LockFreeLogger::set_strategy(std::shared_ptr<ILoggerStrategy> strategy)
{
	if (!strategy) return;
	std::atomic_store_explicit(&m_strategy,strategy, std::memory_order_release);
}
void LockFreeLogger::WorkQueue()
{
	std::queue<std::pair<LogLevel, std::string>> local_queue;
	while (true)
	{
		auto startTimer = std::chrono::steady_clock::now();
		if (m_stop_flag.load(std::memory_order_acquire) && m_ringBuffer.IsEmpty() && local_queue.empty())
			break;
		auto message = m_ringBuffer.Pop();
		if(message)
		{
			if (!message->second.empty())
				local_queue.emplace(message->first, std::move(message->second));
		}
		else
			std::this_thread::yield();

		if (local_queue.size() >= ISXLoggerConfig::LogsForBatch ||
			(m_ringBuffer.IsEmpty() && !local_queue.empty()))
		{
			auto local_strategy = std::atomic_load(&m_strategy);
			if (!local_strategy || !local_strategy->IsValid())
			{
				LogLevel currentLvl = local_strategy ? local_strategy->get_current_level() : LogLevel::PROD;
				auto new_strategy = std::make_shared<ConsoleStrategy>(currentLvl);
				set_strategy(new_strategy);
				local_strategy = std::move(new_strategy);
			}
			while(!local_queue.empty())
			{
				std::string formatted_string =
					local_strategy.get()->SpecificLog(local_queue.front().first, local_queue.front().second);
				if(formatted_string.empty())
				{
					local_queue.pop();
					continue;
				}
				if (!local_strategy.get()->Write(formatted_string))
				{
					LogLevel currentLvl = local_strategy ? local_strategy->get_current_level() : LogLevel::PROD;
					auto new_strategy = std::make_shared<ConsoleStrategy>(currentLvl);
					set_strategy(new_strategy);
					local_strategy = std::move(new_strategy);
					continue;
				}
				local_queue.pop();
			}
			local_strategy.get()->Flush();
		}
	}
}

void LockFreeLogger::Log(LogLevel level, const std::string& message)
{
	auto log = std::make_pair(level, message);
	m_ringBuffer.Push(log);
}

LockFreeLogger::~LockFreeLogger()
{
	m_stop_flag = true;
	if (m_work_thread.joinable())
		m_work_thread.join();
}