#include "ThreadPool.h"

void ThreadPool::StartWorkers() {
    for (int i = 0; i < numOfWorkers; i++) {
        workers.emplace_back(&ThreadPool::WorkerLoop, this);
    }
}

void ThreadPool::WorkerLoop() {
    while (true)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [this] {
                return stop || (!paused && !tasks.empty());
                });

            if (stop && tasks.empty()) {
                return;
            }

            task = std::move(tasks.front());
            tasks.pop();
        }
        task();
    }
}

void ThreadPool::JoinWorkers() {
    for (auto& worker : workers)
    {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

bool ThreadPool::Initialize(int inNumOfWorkers) {
    {
        std::lock_guard<std::mutex> lock(mutex);

        if (!workers.empty() || inNumOfWorkers <= 0) {
            return false;
        }

        stop = false;
        paused = false;

        numOfWorkers = inNumOfWorkers;
    }
    StartWorkers();
    return true;
}

bool ThreadPool::Terminate() {
    {
        std::lock_guard<std::mutex> lock(mutex);

        if (stop) {
            return true;
        }

        stop = true;
    }

    cv.notify_all();
    JoinWorkers();
    {
        std::lock_guard<std::mutex> lock(mutex);
        workers.clear();
    }
    return true;
}

bool ThreadPool::AddTask(std::function<void()> task) {
    std::lock_guard<std::mutex> lock(mutex);

    if (stop || workers.empty()) {
        return false;
    }

    tasks.push(std::move(task));
    cv.notify_one();
    return true;
}

bool ThreadPool::Pause() {
    std::lock_guard<std::mutex> lock(mutex);

    if (stop) {
        return false;
    }

    paused = true;
    return true;
}

bool ThreadPool::Resume() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (stop) {
            return false;
        }

        paused = false;
    }

    cv.notify_all();
    return true;
}