#pragma once
#include <condition_variable>
#include <functional>
#include <future>
#include <queue>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <stdexcept>
#include <type_traits>
#include "ILogger.h"

enum class Priority { Low = 0, Normal = 1, High = 2 };

struct Task {
    int id;
    Priority priority;
    std::function<void()> func;

    explicit Task(int id = 0, Priority priority = Priority::Normal, std::function<void()> func = [] {})
        : id(id), priority(priority), func(std::move(func)) {}

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    Task(Task&&) = default;
    Task& operator=(Task&&) = default;
};

struct TaskComparator {
    bool operator()(const Task& a, const Task& b) const {
        return static_cast<int>(a.priority) < static_cast<int>(b.priority);
    }
};

class TasksQueue {
    std::priority_queue<Task, std::vector<Task>, TaskComparator> m_tasks;
public:
    TasksQueue() = default;
    ~TasksQueue();

    TasksQueue(const TasksQueue&) = delete;
    TasksQueue& operator=(const TasksQueue&) = delete;
    TasksQueue(TasksQueue&&) = delete;
    TasksQueue& operator=(TasksQueue&&) = delete;

    bool empty();
    std::size_t size();
    void clear();
    bool pop(Task& task);
    bool push(int taskId, Priority priority, std::function<void()> func);
};

class ThreadPool {
    std::vector<std::thread> m_workers;
    TasksQueue m_queue;

    std::mutex m_queue_mutex;
    std::condition_variable m_queue_cv;

    std::mutex m_pool_state_mutex; // for stop/terminate/pause

    std::atomic<bool> m_is_terminated{false};
    std::atomic<bool> m_is_initialized{false};
    std::atomic<bool> m_is_paused{false};

    std::atomic<int> m_total_tasks{0};

    ILogger* m_logger = nullptr;

    bool is_working_unsafe() const;
    void routine(int worker_id);
    void log(LogLevel level, const std::string& msg);

public:
    ~ThreadPool() { terminate(); }
    bool is_working();
    void set_logger(ILogger* logger);
    void initialize(int workerNumber);
    
    template<typename F, typename... Args>
    auto add_task(Priority priority, F&& func, Args&&... args)
        -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>;

    template<typename F, typename... Args>
    auto add_task(F&& func, Args&&... args)
        -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>;

    void pause();
    void unpause();
    void terminate();
};

/*
Return value is std::future<R> where R is the return type of func(args...)
Priority defaults to "Normal"

API:
Fire-and-forget — handle one client connection
    pool.add_task(handle_connection, client_fd);

Getting the result
    auto fut = pool.add_task(Priority::High, parse_command, raw_line);
    SmtpReply reply = fut.get();
*/

template<typename F, typename... Args>
auto ThreadPool::add_task(Priority priority, F&& func, Args&&... args)
    -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>> {
    using R = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;

    if (!m_is_initialized.load() || m_is_terminated.load()) {
        log(LogLevel::PROD, "[ThreadPool] Warning: Failed to add task. Pool not ready.");
        std::promise<R> p;
        p.set_exception(std::make_exception_ptr(std::runtime_error("ThreadPool not ready")));
        return p.get_future();
    }

    auto pkg = std::make_shared<std::packaged_task<R()>>(
        std::bind(std::forward<F>(func), std::forward<Args>(args)...)
    );
    auto fut = pkg->get_future();

    int taskId = m_total_tasks.fetch_add(1);
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_queue.push(taskId, priority, [pkg]() { (*pkg)(); });
    }
    m_queue_cv.notify_one();

    log(LogLevel::TRACE, "[ThreadPool] Task " + std::to_string(taskId) + " added.");
    return fut;
}

template<typename F, typename... Args>
auto ThreadPool::add_task(F&& func, Args&&... args)
    -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>> {
    return add_task(Priority::Normal, std::forward<F>(func), std::forward<Args>(args)...);
}
