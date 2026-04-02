#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

#include "MailCommand.hpp"

class CommandQueue
{
public:
    void push(const MailCommand& command);
    std::optional<MailCommand> pop();
    void stop();

private:
    std::queue<MailCommand> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_stopped = false;
    
};