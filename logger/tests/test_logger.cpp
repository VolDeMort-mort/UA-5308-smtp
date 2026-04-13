#include <memory>
#include <filesystem>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <unistd.h>

#include <gtest/gtest.h>

#include "ILogger.h"
#include "Logger.h"
#include "FileStrategy.h"
#include "ConsoleStrategy.h"

static std::string logPath() {
    return "/tmp/log_" + std::to_string(getpid()) + ".txt";
}

static std::string stressLogPath() {
    return "/tmp/stress_log_" + std::to_string(getpid()) + ".txt";
}

static std::string switchLogPath() {
    return "/tmp/switch_log_" + std::to_string(getpid()) + ".txt";
}

class LoggerClass : public ::testing::Test {
protected:
    std::unique_ptr<Logger> logger;

    void SetUp() override
    {
        std::filesystem::remove(logPath());

        logger = std::make_unique<Logger>(
            std::make_unique<FileStrategy>(TRACE, logPath())
        );
    }

    void TearDown() override
    {
        logger.reset();
        std::filesystem::remove(logPath());
        std::filesystem::remove(logPath() + ".old");
    }
};

class FileStrategyClass : public ::testing::Test {
protected:
    std::unique_ptr<FileStrategy> strategy;

    void SetUp() override
    {
        std::filesystem::remove(logPath());

        strategy = std::make_unique<FileStrategy>(TRACE, logPath());
    }

    void TearDown() override
    {
        strategy.reset();
        std::filesystem::remove(logPath());
        std::filesystem::remove(logPath() + ".old");
    }
};

class BrokenFileStrategy : public FileStrategy
{
public:
    using FileStrategy::FileStrategy;
    bool IsValid() override { return false; }
};

TEST_F(LoggerClass, DestructorTest)
{
    logger->Log(PROD, "First Log");
    logger->Log(PROD, "Second Log");

    logger.reset();

    FileStrategy reader(TRACE, logPath());
    auto logs = reader.Read(2);

    ASSERT_EQ(logs.size(), 2);
}

TEST_F(FileStrategyClass, FileCheck)
{
    EXPECT_TRUE(strategy->IsValid());
}

TEST_F(FileStrategyClass, WriteFile)
{
    EXPECT_TRUE(strategy->Write("log message\n"));
}

TEST_F(FileStrategyClass, RotateFile)
{
    EXPECT_NO_THROW(strategy->Rotate());
}

TEST_F(FileStrategyClass, FlushFile)
{
    strategy->Write("message\n");

    auto file_logs = strategy->Read(1);
    EXPECT_EQ(file_logs.size(), 0);

    strategy->Flush();

    file_logs = strategy->Read(1);
    EXPECT_EQ(file_logs.size(), 1);
}

TEST_F(LoggerClass, FewLogsTest)
{
    logger->Log(PROD, "First Log");
    logger->Log(PROD, "Second Log");

    auto logs = logger->Read(2);

    EXPECT_TRUE(logs[0].find("First Log") != std::string::npos);
    EXPECT_TRUE(logs[1].find("Second Log") != std::string::npos);
}

TEST_F(LoggerClass, FiftyLogsTest)
{
    for (int i = 0; i < 49; ++i)
        logger->Log(PROD, "Log #" + std::to_string(i + 1));

    logger->Log(PROD, "Log #50");

    auto logs = logger->Read(50);

    EXPECT_TRUE(logs[0].find("Log #1") != std::string::npos);
    EXPECT_TRUE(logs[49].find("Log #50") != std::string::npos);
}

TEST_F(LoggerClass, TimeoutLogTest)
{
    logger->Log(PROD, "First Log");
    logger->Log(PROD, "Second Log");

    std::this_thread::sleep_for(std::chrono::milliseconds(2100));

    EXPECT_GT(std::filesystem::file_size(logPath()), 0);
}

TEST_F(LoggerClass, OrderTest)
{
    std::thread t1([this]() {
        for (int i = 0; i < 50; ++i)
            logger->Log(PROD, "One #" + std::to_string(i + 1));
    });

    std::thread t2([this]() {
        for (int i = 0; i < 50; ++i)
            logger->Log(PROD, "Two #" + std::to_string(i + 1));
    });

    t1.join();
    t2.join();

    auto read = logger->Read(100);
    EXPECT_EQ(read.size(), 100);
}

TEST_F(LoggerClass, StressTest)
{
    std::filesystem::remove(stressLogPath());

    auto stress_logger = std::make_unique<Logger>(
        std::make_unique<FileStrategy>(TRACE, stressLogPath())
    );

    std::vector<std::thread> threads;

    for (size_t i = 0; i < 10; ++i)
    {
        threads.emplace_back([&stress_logger]() {
            for (int j = 0; j < 100; ++j)
                stress_logger->Log(PROD, "Logger working");

            stress_logger->Log(DEBUG, "Logger Finished");
        });
    }

    for (auto& t : threads)
        t.join();

    auto read = stress_logger->Read(1010);
    stress_logger.reset();
    std::filesystem::remove(stressLogPath());
    std::filesystem::remove(stressLogPath() + ".old");
    EXPECT_EQ(read.size(), 1010);
}

TEST_F(LoggerClass, EmptyFileTest)
{
    auto read = logger->Read(10);
    EXPECT_EQ(read.size(), 0);

    auto search = logger->Search(PROD, 10, 1000);
    EXPECT_EQ(search.size(), 0);
}

TEST_F(LoggerClass, SearchTest)
{
    logger->Log(PROD, "First Log");
    logger->Log(TRACE, "LOG MESSAGE");
    logger->Log(DEBUG, "SEARCH");
    logger->Log(TRACE, "Log");
    logger->Log(TRACE, "Hello 123");
    logger->Log(PROD, "Server");
    logger->Log(TRACE, "LAST Log");
    logger->Log(PROD, "Example");

    auto search_logs = logger->Search(PROD, 2, 8);

    EXPECT_TRUE(search_logs[0].find("First Log") != std::string::npos);
    EXPECT_TRUE(search_logs[1].find("Server") != std::string::npos);
}

TEST_F(LoggerClass, SetStrategyTest)
{
    for (int i = 0; i < 100; ++i)
    {
        logger->Log(PROD, "Log #" + std::to_string(i + 1));

        if (i == 90)
        {
            logger->set_strategy(
                std::make_unique<ConsoleStrategy>()
            );
        }
    }

    auto read = logger->Read(100);
    EXPECT_EQ(read.size(), 0);
}

TEST(LoggerConsole, ConsoleLevelOutput)
{
    Logger logger(std::make_unique<ConsoleStrategy>());

    logger.Log(TRACE, "CONSOLE TRACE MESSAGE");
    logger.Log(DEBUG, "CONSOLE DEBUG MESSAGE");
    logger.Log(PROD, "CONSOLE PROD MESSAGE");
    logger.Log(NONE, "CONSOLE NONE MESSAGE");
}

TEST(LoggerSwitchStrategies, MultipleSwitches)
{
    std::filesystem::remove(switchLogPath());

    Logger logger(std::make_unique<ConsoleStrategy>());

    logger.set_strategy(std::make_unique<FileStrategy>(TRACE, switchLogPath()));
    logger.Log(TRACE, "FILE MESSAGE");

    auto file_logs = logger.Read(1);
    EXPECT_EQ(file_logs.size(), 1);

    logger.set_strategy(std::make_unique<BrokenFileStrategy>(TRACE, switchLogPath()));
    logger.Log(TRACE, "CONSOLE MESSAGE");

    auto console_logs = logger.Read(1);
    std::filesystem::remove(switchLogPath());
    std::filesystem::remove(switchLogPath() + ".old");
    EXPECT_EQ(console_logs.size(), 0);
}

TEST(LoggerPath, FileNotValid)
{
    Logger logger(std::make_unique<BrokenFileStrategy>(PROD, logPath()));
    logger.Log(PROD, "CONSOLE MESSAGE");

    auto console_logs = logger.Read(1);
    EXPECT_EQ(console_logs.size(), 0);
}
