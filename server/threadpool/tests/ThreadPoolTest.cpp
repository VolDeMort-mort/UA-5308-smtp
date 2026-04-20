#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "ThreadPool.h"
#include "Logger.h"
#include "ILoggerStrategy.h"

struct Captured {
    std::vector<std::string> messages;
    std::mutex mtx;

    bool Contains(const std::string& sub) {
        std::lock_guard<std::mutex> lock(mtx);
        for (const auto& m : messages)
            if (m.find(sub) != std::string::npos) return true;
        return false;
    }

    int CountContaining(const std::string& sub) {
        std::lock_guard<std::mutex> lock(mtx);
        int n = 0;
        for (const auto& m : messages)
            if (m.find(sub) != std::string::npos) ++n;
        return n;
    }
};

class TestStrategy : public ILoggerStrategy {
public:
    std::shared_ptr<Captured> m_data;
    explicit TestStrategy(std::shared_ptr<Captured> d) : ILoggerStrategy(PROD), m_data(std::move(d)) {}

    std::string SpecificLog(LogLevel, const std::string& msg) override { return msg + "\n"; }
    bool IsValid() override { return true; }
    bool Write(const std::string& msg) override {
        std::lock_guard<std::mutex> lock(m_data->mtx);
        m_data->messages.push_back(msg);
        return true;
    }
    void Flush() override {}
    std::string get_name() const override { return "Test"; }
};

TEST(ThreadPool, GivenInitializedPool_WhenTaskAdded_FutureResolvesCorrectly) {
    ThreadPool pool;
    pool.initialize(2);
    auto fut = pool.add_task([] { return 42; });
    EXPECT_EQ(fut.get(), 42);
    pool.terminate();
}

TEST(ThreadPool, GivenInitializedPool_WhenMultipleTasksAdded_AllFuturesResolve) {
    ThreadPool pool;
    pool.initialize(2);
    std::vector<std::future<int>> futs;
    for (int i = 0; i < 10; ++i)
        futs.push_back(pool.add_task([i] { return i * i; }));
    for (int i = 0; i < 10; ++i)
        EXPECT_EQ(futs[i].get(), i * i);
    pool.terminate();
}

TEST(ThreadPool, GivenUninitializedPool_WhenTaskAdded_FutureThrows) {
    ThreadPool pool;
    auto fut = pool.add_task([] { return 1; });
    EXPECT_THROW(fut.get(), std::runtime_error);
}

TEST(ThreadPool, GivenPausedPool_WhenUnpaused_QueuedTaskCompletes) {
    ThreadPool pool;
    pool.initialize(2);
    pool.pause();
    auto fut = pool.add_task([] { return 99; });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_NE(fut.wait_for(std::chrono::milliseconds(0)), std::future_status::ready);
    pool.unpause();
    EXPECT_EQ(fut.get(), 99);
    pool.terminate();
}

TEST(ThreadPool, GivenTerminatedPool_WhenTaskAdded_FutureThrows) {
    ThreadPool pool;
    pool.initialize(2);
    pool.terminate();
    auto fut = pool.add_task([] { return 1; });
    EXPECT_THROW(fut.get(), std::runtime_error);
}

TEST(ThreadPoolLogger, GivenPoolWithLogger_WhenTaskAdded_AddedMessageLogged) {
    auto cap = std::make_shared<Captured>();
    {
        Logger logger(std::make_unique<TestStrategy>(cap));
        ThreadPool pool;
        pool.set_logger(&logger);
        pool.initialize(2);
        pool.add_task([] { return 1; }).get();
        pool.terminate();
    }
    EXPECT_TRUE(cap->Contains("Task 0 added"));
}

TEST(ThreadPoolLogger, GivenPoolWithLogger_WhenWorkerExecutesTask_ExecutionMessagesLogged) {
    auto cap = std::make_shared<Captured>();
    {
        Logger logger(std::make_unique<TestStrategy>(cap));
        ThreadPool pool;
        pool.set_logger(&logger);
        pool.initialize(2);
        pool.add_task([] { return 1; }).get();
        pool.terminate();
    }
    EXPECT_TRUE(cap->Contains("executing task"));
    EXPECT_TRUE(cap->Contains("finished task"));
}

TEST(ThreadPoolLogger, GivenPoolWithLogger_WhenTerminated_TerminationMessagesLogged) {
    auto cap = std::make_shared<Captured>();
    {
        Logger logger(std::make_unique<TestStrategy>(cap));
        ThreadPool pool;
        pool.set_logger(&logger);
        pool.initialize(2);
        pool.terminate();
    }
    EXPECT_TRUE(cap->Contains("Terminating"));
    EXPECT_TRUE(cap->Contains("Terminated"));
}

TEST(ThreadPoolLogger, GivenPoolWithLogger_WhenPausedAndUnpaused_StateMessagesLogged) {
    auto cap = std::make_shared<Captured>();
    {
        Logger logger(std::make_unique<TestStrategy>(cap));
        ThreadPool pool;
        pool.set_logger(&logger);
        pool.initialize(2);
        pool.pause();
        pool.unpause();
        pool.terminate();
    }
    EXPECT_TRUE(cap->Contains("Paused"));
    EXPECT_TRUE(cap->Contains("Unpaused"));
}

TEST(ThreadPoolLogger, GivenPoolWithLogger_WhenConcurrentTasksAdded_AllTasksLogged) {
    auto cap = std::make_shared<Captured>();
    {
        Logger logger(std::make_unique<TestStrategy>(cap));
        ThreadPool pool;
        pool.set_logger(&logger);
        pool.initialize(4);
        std::vector<std::future<int>> futs;
        for (int i = 0; i < 100; ++i)
            futs.push_back(pool.add_task([i] { return i; }));
        for (auto& f : futs) f.get();
        pool.terminate();
    }
    EXPECT_EQ(cap->CountContaining("added"), 100);
}
