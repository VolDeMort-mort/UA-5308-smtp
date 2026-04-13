#include "Logger.h"
#include <chrono>
#include <iostream>
#include <thread>
#include "ConsoleStrategy.h"
#include "FileStrategy.h"

Logger::Logger(std::unique_ptr<ILoggerStrategy> strategy)
{
    set_strategy(std::move(strategy));
    m_running_flag = true;
    m_flush = false;
    m_flush_done = false;
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
    bool notify = false;

    {
        std::lock_guard<std::mutex> lock(m_queue_mtx);
        m_queue.push(message);

        if (m_queue.size() >= 50)
            notify = true;
    }

    if (notify)
        m_cv.notify_all();
}

void Logger::WorkQueue()
{
    std::queue<std::string> local_queue;

    while (true)
    {
        {
            std::unique_lock<std::mutex> lock(m_queue_mtx);

            m_cv.wait_for(lock, std::chrono::seconds(2), [this] {
                return !m_queue.empty() || !m_running_flag || m_flush.load();
            });

            if (!m_running_flag && m_queue.empty())
                break;

            while (!m_queue.empty())
            {
                local_queue.push(std::move(m_queue.front()));
                m_queue.pop();
            }
        }

        if (!local_queue.empty())
        {
            if (!m_strategy->IsValid())
                m_strategy = std::make_unique<ConsoleStrategy>();

            while (!local_queue.empty())
            {
                if (!m_strategy->Write(local_queue.front()))
                {
                    m_strategy = std::make_unique<ConsoleStrategy>();
                    continue;
                }
                local_queue.pop();
            }

            m_strategy->Flush();
        }

        {
            std::lock_guard<std::mutex> lock(m_queue_mtx);
            if (m_queue.empty())
            {
                m_flush_done = true;
                m_cv.notify_all();
            }
        }
    }
}

void Logger::Log(LogLevel lvl, const std::string& msg)
{
    if (!m_strategy) return;

    std::string message = m_strategy->SpecificLog(lvl, msg);
    PushToQueue(message);
}

std::vector<std::string> Logger::Read(size_t limit)
{
    {
        std::unique_lock<std::mutex> lock(m_queue_mtx);

        m_flush = true;
        m_flush_done = false;

        m_cv.notify_all();

        m_cv.wait(lock, [this] {
            return m_queue.empty() && m_flush_done;
        });

        m_flush = false;
        m_flush_done = false;

        m_cv.notify_all();
    }

    m_strategy->Flush();

    auto* readable = dynamic_cast<IReadable*>(m_strategy.get());
    if (!readable) return {};

    return readable->Read(limit);
}

std::vector<std::string> Logger::Search(LogLevel lvl, size_t limit, int read_n)
{
    {
        std::unique_lock<std::mutex> lock(m_queue_mtx);

        m_flush = true;
        m_flush_done = false;

        m_cv.notify_all();

        m_cv.wait(lock, [this] {
            return m_queue.empty() && m_flush_done;
        });

        m_flush = false;
        m_flush_done = false;

        m_cv.notify_all();
    }

    m_strategy->Flush();

    auto* searchable = dynamic_cast<IReadable*>(m_strategy.get());
    if (!searchable) return {};

    return searchable->Search(lvl, limit, read_n);
}

Logger::~Logger()
{
    m_running_flag = false;
    m_cv.notify_all();

    if (m_work_thread.joinable())
        m_work_thread.join();
}
