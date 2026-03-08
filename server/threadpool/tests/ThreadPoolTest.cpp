#include <gtest/gtest.h>
#include "../ThreadPool/ThreadPool.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

TEST(ThreadPoolTest, ExecutesAllTasks)
{
    ThreadPool pool;
    ASSERT_TRUE(pool.Initialize(4));

    std::atomic<int> counter = 0;

    for (int i = 0; i < 100; ++i)
    {
        pool.AddTask([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
            });
    }

    pool.Terminate();

    EXPECT_EQ(counter.load(), 100);
}

TEST(ThreadPoolTest, TerminateFinishesAllQueuedTasks)
{
    ThreadPool pool;
    ASSERT_TRUE(pool.Initialize(2));

    std::atomic<int> counter = 0;

    for (int i = 0; i < 50; ++i)
    {
        pool.AddTask([&counter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            counter.fetch_add(1, std::memory_order_relaxed);
            });
    }

    pool.Terminate();

    EXPECT_EQ(counter.load(), 50);
}

TEST(ThreadPoolTest, AddTaskAfterTerminateReturnsFalse)
{
    ThreadPool pool;
    ASSERT_TRUE(pool.Initialize(2));

    pool.Terminate();

    bool result = pool.AddTask([]() {});
    EXPECT_FALSE(result);
}

TEST(ThreadPoolTest, CanRestartAfterTerminate)
{
    ThreadPool pool;

    ASSERT_TRUE(pool.Initialize(2));
    pool.Terminate();

    ASSERT_TRUE(pool.Initialize(2));

    std::atomic<int> counter = 0;

    pool.AddTask([&counter]() {
        counter.fetch_add(1, std::memory_order_relaxed);
        });

    pool.Terminate();

    EXPECT_EQ(counter.load(), 1);
}

TEST(ThreadPoolTest, ConcurrentAddTask)
{
    ThreadPool pool;
    ASSERT_TRUE(pool.Initialize(4));

    std::atomic<int> counter = 0;

    const int threadsCount = 10;
    const int tasksPerThread = 50;

    std::vector<std::thread> threads;

    for (int i = 0; i < threadsCount; ++i)
    {
        threads.emplace_back([&]() {
            for (int j = 0; j < tasksPerThread; ++j)
            {
                pool.AddTask([&counter]() {
                    counter.fetch_add(1, std::memory_order_relaxed);
                    });
            }
            });
    }

    for (auto& t : threads)
        t.join();

    pool.Terminate();

    EXPECT_EQ(counter.load(), threadsCount * tasksPerThread);
}

TEST(ThreadPoolTest, TerminateIsIdempotent)
{
    ThreadPool pool;
    ASSERT_TRUE(pool.Initialize(2));

    EXPECT_TRUE(pool.Terminate());
    EXPECT_TRUE(pool.Terminate());
}

TEST(ThreadPoolTest, InitializeTwiceFails)
{
    ThreadPool pool;

    ASSERT_TRUE(pool.Initialize(2));
    EXPECT_FALSE(pool.Initialize(4));

    pool.Terminate();
}

TEST(ThreadPoolTest, InitializeWithZeroWorkersFails)
{
    ThreadPool pool;
    EXPECT_FALSE(pool.Initialize(0));
}

TEST(ThreadPoolTest, AddTaskBeforeInitializeReturnsFalse)
{
    ThreadPool pool;

    bool result = pool.AddTask([]() {});
    EXPECT_FALSE(result);
}

TEST(ThreadPoolTest, TasksExecuteConcurrently)
{
    ThreadPool pool;
    ASSERT_TRUE(pool.Initialize(4));

    std::atomic<int> running = 0;
    std::atomic<int> maxRunning = 0;

    for (int i = 0; i < 20; ++i)
    {
        pool.AddTask([&]() {
            int current = ++running;

            maxRunning.store(
                std::max(maxRunning.load(), current),
                std::memory_order_relaxed);

            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            --running;
            });
    }

    pool.Terminate();

    EXPECT_GT(maxRunning.load(), 1);
}

TEST(ThreadPoolTest, TerminateWhileAddingTasks)
{
    ThreadPool pool;
    ASSERT_TRUE(pool.Initialize(4));

    std::atomic<int> counter = 0;
    std::atomic<bool> stopAdding = false;

    std::thread adder([&]() {
        while (!stopAdding.load())
        {
            pool.AddTask([&]() {
                counter.fetch_add(1, std::memory_order_relaxed);
                });
        }
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    pool.Terminate();
    stopAdding = true;

    adder.join();

    EXPECT_GE(counter.load(), 0);
}

TEST(ThreadPoolTest, DestructorStopsPool)
{
    std::atomic<int> counter = 0;

    {
        ThreadPool pool;
        ASSERT_TRUE(pool.Initialize(2));

        for (int i = 0; i < 20; ++i)
        {
            pool.AddTask([&]() {
                counter.fetch_add(1, std::memory_order_relaxed);
                });
        }
    }

    EXPECT_EQ(counter.load(), 20);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}