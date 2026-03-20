#include "ThreadPool.h"

TasksQueue::~TasksQueue() { clear(); }

bool TasksQueue::empty() {
    return m_tasks.empty();
}

std::size_t TasksQueue::size() {
    return m_tasks.size();
}

void TasksQueue::clear() {
    while (!m_tasks.empty()) {
        m_tasks.pop();
    }
}

bool TasksQueue::pop(Task& task) {
    if (m_tasks.empty()) return false;
    task = std::move(const_cast<Task&>(m_tasks.top()));
    m_tasks.pop();
    return true;
}

bool TasksQueue::push(int taskId, Priority priority, std::function<void()> func) {
    Task newTask(taskId, priority, std::move(func));
    m_tasks.push(std::move(newTask));
    return true;
}

bool ThreadPool::is_working_unsafe() const {
    return m_is_initialized.load() && !m_is_terminated.load();
}

bool ThreadPool::is_working() {
    std::lock_guard<std::mutex> lock(m_pool_state_mutex);
    return is_working_unsafe();
}

void ThreadPool::set_logger(Logger* logger) {
    m_logger = logger;
}

void ThreadPool::initialize(int workerNumber) {
    std::lock_guard<std::mutex> lock(m_pool_state_mutex);
    if (m_is_initialized) return;

    m_workers.reserve(workerNumber);
    for (int i = 0; i < workerNumber; ++i) {
        m_workers.emplace_back(&ThreadPool::routine, this, i);
    }
    m_is_initialized = true;
    m_is_paused = false;
    m_is_terminated = false;
}

void ThreadPool::routine(int worker_id) {
    while (true) {
        Task task;
        bool is_task_popped = false;
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_queue_cv.wait(lock, [&] {
                return m_is_terminated.load() || (!m_is_paused.load() && !m_queue.empty());
            });

            if (m_is_terminated.load()) break;
            if (m_is_paused.load() || m_queue.empty()) continue;

            is_task_popped = m_queue.pop(task);
        }

        if (is_task_popped) {
            if (m_logger) m_logger->Log(LogLevel::TRACE,
                "[Worker " + std::to_string(worker_id) + "] executing task " + std::to_string(task.id));
            task.func();
            if (m_logger) m_logger->Log(LogLevel::TRACE,
                "[Worker " + std::to_string(worker_id) + "] finished task " + std::to_string(task.id));
        }
    }
}

void ThreadPool::pause() {
    std::lock_guard<std::mutex> lock(m_pool_state_mutex);
    if (!m_is_initialized.load() || m_is_terminated.load()) return;
    m_is_paused = true;
    if (m_logger) m_logger->Log(LogLevel::PROD, "[ThreadPool] Paused.");
}

void ThreadPool::unpause() {
    bool should_notify = false;
    {
        std::lock_guard<std::mutex> lock(m_pool_state_mutex);
        if (!m_is_initialized.load() || m_is_terminated.load() || !m_is_paused.load()) {
            return;
        }
        m_is_paused = false;
        should_notify = true;
        if (m_logger) m_logger->Log(LogLevel::PROD, "[ThreadPool] Unpaused.");
    } // releasing m_pool_state_mutex

    if (should_notify) {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_queue_cv.notify_all();
    }
}

void ThreadPool::terminate() {
    bool should_notify = false;
    {
        std::lock_guard<std::mutex> lock(m_pool_state_mutex);
        if (!m_is_initialized.load() || m_is_terminated.load()) return;
        m_is_terminated = true;
        m_is_paused = false;
        should_notify = true;
        if (m_logger) m_logger->Log(LogLevel::PROD, "[ThreadPool] Terminating...");
    }

    if (should_notify) {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_queue_cv.notify_all();
    }

    for (auto& worker : m_workers) {
        if (worker.joinable()) worker.join();
    }

    std::lock_guard<std::mutex> lock(m_pool_state_mutex);
    m_workers.clear();
    m_queue.clear();
    m_is_initialized = false;
    m_is_paused = false;
    m_total_tasks = 0;
    if (m_logger) m_logger->Log(LogLevel::PROD, "[ThreadPool] Terminated.");
}
