#include "CommandQueue.hpp"

void CommandQueue::push(const MailCommand& command)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(command);
    }
    m_condition.notify_one();
}

std::optional<MailCommand> CommandQueue::pop()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait(lock, [this] { return !m_queue.empty() || m_stopped; });

    if (m_stopped && m_queue.empty())
        return std::nullopt;

    MailCommand command = std::move(m_queue.front());    
    m_queue.pop();
    return command;
}

void CommandQueue::stop()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopped = true;
    }
    m_condition.notify_all();
}