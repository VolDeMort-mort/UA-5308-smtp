#include <memory>
#include <filesystem>
#include <chrono>
#include <iostream>

#include <gtest/gtest.h>

#include "ILogger.h"
#include "Logger.h"
#include "FileStrategy.h"
#include "ConsoleStrategy.h"
#include "LoggerConfig.h"

class LoggerClass : public ::testing::Test {
protected:
	std::shared_ptr<Logger> logger;
	void SetUp() override
	{
		std::filesystem::remove("log.txt");
		logger = std::make_shared<Logger>(std::make_shared<FileStrategy>(PROD));
	}
};
class FileStrategyClass : public ::testing::Test {
protected:
	std::shared_ptr<FileStrategy> strategy;
	void SetUp() override
	{
		std::filesystem::remove("log.txt");
		strategy = std::make_shared<FileStrategy>(PROD);
	}
};
class BrokenFileStrategy : public FileStrategy
{
public:
	using FileStrategy::FileStrategy;
	bool IsValid() override { return false; };
};

TEST_F(LoggerClass, DestructorTest)
{
	logger->Log(PROD, "First Log");
	logger->Log(PROD, "Second Log");

	logger.reset();// call destructor to see all logs will be written to file
	
	FileStrategy reader(PROD);
	auto logs = reader.Read(2);

	ASSERT_EQ(logs.size(), 2);
}

TEST_F(FileStrategyClass, FileCheck)
{
	EXPECT_TRUE(strategy->IsValid());
}

TEST_F(FileStrategyClass, WriteFile)
{
	EXPECT_TRUE(strategy->Write("log message"));
}
TEST_F(FileStrategyClass, RotateFile)
{
	EXPECT_NO_THROW(strategy->Rotate());
}
TEST_F(FileStrategyClass, FlushFile)
{
	strategy->Write("message");
	auto file_logs = strategy->Read(1);
	EXPECT_EQ(file_logs.size(), 0);

	strategy->IsValid();
	strategy->Flush();
	file_logs = strategy->Read(1);
	EXPECT_EQ(file_logs.size(), 1);
	EXPECT_EQ(file_logs[0], "message");
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
	{
		logger->Log(PROD, "Log #" + std::to_string(i + 1));
	}
	EXPECT_EQ(std::filesystem::file_size(ISXLoggerConfig::FilePath), 0);
	logger->Log(PROD, "Log #50");

	auto logs = logger->Read(50);
	EXPECT_TRUE(logs[0].find("Log #1") != std::string::npos);
	EXPECT_TRUE(logs[49].find("Log #50") != std::string::npos);
}

TEST_F(LoggerClass, TimeoutLogTest)
{
	logger->Log(PROD, "First Log");
	logger->Log(PROD, "Second Log");
	EXPECT_EQ(std::filesystem::file_size(ISXLoggerConfig::FilePath), 0);
	std::this_thread::sleep_for(std::chrono::milliseconds(2100));
	EXPECT_GT(std::filesystem::file_size(ISXLoggerConfig::FilePath), 0);
}
TEST_F(LoggerClass, OrderTest)
{
	std::thread t1([this]() {
		for (int i = 0; i < 50; ++i) {
			logger->Log(PROD, "One #" + std::to_string(i + 1));
		}
	});
	std::thread t2([this]() {
		for (int i = 0; i < 50; ++i) {
			logger->Log(PROD, "Two #" + std::to_string(i + 1));
		}
	});
	t1.join();
	t2.join();

	auto read = logger->Read(100);
	EXPECT_EQ(read.size(), 100);
	int nextOne = 1;
	int nextTwo = 1;

	for (const auto& log : read) {
		std::string expectOne = "One #" + std::to_string(nextOne);
		std::string expectTwo = "Two #" + std::to_string(nextTwo);

		if (log.find(expectOne) != std::string::npos)
			++nextOne;
		else if (log.find(expectTwo) != std::string::npos)
			++nextTwo;
	}

	EXPECT_EQ(nextOne - 1, 50);
	EXPECT_EQ(nextTwo - 1, 50);
}
TEST_F(LoggerClass, StressTest)
{
	std::shared_ptr<Logger> stress_logger;
	stress_logger = std::make_shared<Logger>(std::make_shared<FileStrategy>(DEBUG));
	std::vector<std::thread> threads;
	for (size_t i = 0; i < 10; ++i) {
		threads.emplace_back(std::thread([this,&stress_logger]() {
			int j = 0;
			while (j < 100)
			{
				stress_logger->Log(PROD, "Logger working");
				++j;
			}
			stress_logger->Log(DEBUG, "Logger Finished");
			}));
	}
	for (auto& t : threads) {
		if (t.joinable())
			t.join();
	}
	auto read = stress_logger->Read(1010);
	EXPECT_EQ(read.size(), 1010);
	auto search_logs = stress_logger->Search(DEBUG, 10, 1010);
	EXPECT_EQ(search_logs.size(), 10);
}
TEST_F(LoggerClass, EmptyFileTest)
{
	auto read = logger->Read(10);
	EXPECT_EQ(read.size(),2);
	auto search = logger->Search(PROD,10,1000);
	EXPECT_EQ(search.size(), 0);
}
TEST_F(LoggerClass, SearchTest)
{
	logger->set_level(TRACE);
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

	auto search_debug_logs = logger->Search(DEBUG, 2, 8);
	EXPECT_TRUE(search_debug_logs[0].find("SEARCH") != std::string::npos);
	EXPECT_TRUE(search_debug_logs.size() == 1);

	auto search_trace__logs = logger->Search(TRACE, 3, 8);
	EXPECT_TRUE(search_trace__logs[0].find("LOG MESSAGE") != std::string::npos);
	EXPECT_TRUE(search_trace__logs[1].find("Log") != std::string::npos);
	EXPECT_TRUE(search_trace__logs[2].find("Hello 123") != std::string::npos);
}
TEST_F(LoggerClass, SetStrategyTest)
{
	for (int i = 0; i < 100; ++i)
	{
		logger->Log(PROD, "Log #" + std::to_string(i + 1));
		if (i == 90)
		{
			auto read = logger->Read(100);
			EXPECT_EQ(read.size(), 93);
			logger->set_strategy(std::make_unique<ConsoleStrategy>(PROD));
		}
	}
	auto read = logger->Read(100);
	EXPECT_EQ(read.size(), 0);
}
TEST(LoggerConsole, ConsoleLevelOutput)
{
	Logger logger(std::make_shared<ConsoleStrategy>(TRACE));
	logger.Log(TRACE, "CONSOLE TRACE MESSAGE");
	logger.Log(DEBUG, "CONSOLE DEBUG MESSAGE");
	logger.Log(PROD, "CONSOLE PROD MESSAGE");
	logger.Log(NONE, "CONSOLE NONE MESSAGE");
}
TEST(LoggerSwitchStrategies, MultipleSwitches)
{
	Logger logger(std::make_shared<ConsoleStrategy>(TRACE));

	logger.set_strategy(std::make_shared<FileStrategy>(TRACE));

	logger.Log(TRACE, "FILE MESSAGE");

	auto file_logs = logger.Read(1);
	EXPECT_EQ(file_logs.size(), 1);

	logger.set_strategy(std::make_shared<BrokenFileStrategy>(TRACE));

	logger.Log(TRACE, "CONSOLE MESSAGE");

	auto console_logs = logger.Read(1);
	EXPECT_EQ(console_logs.size(), 0);
}

TEST(LoggerPath, FileNotValid)
{
	Logger logger(std::make_shared<BrokenFileStrategy>(PROD));
	logger.Log(PROD, "CONSOLE MESSAGE");

	auto console_logs = logger.Read(1);
	EXPECT_EQ(console_logs.size(), 0);
}

TEST(Logger, MaxThreadSpeedTest)
{
	int thread_count = std::thread::hardware_concurrency();
	auto start = std::chrono::high_resolution_clock::now();
	auto file_strategy = std::make_shared<FileStrategy>(PROD);
	std::unique_ptr<ILogger> loggerTest = std::make_unique<Logger>(file_strategy);

	std::vector<std::thread> threads;
	for (size_t i = 0; i < thread_count; ++i)
	{
		threads.emplace_back(std::thread([&loggerTest](){
				int j = 0;
				std::string msg = "SpeedTest Log Message";
				while (j < 100000)
				{
					loggerTest->Log(PROD, msg);
					++j;
				}
			}));
	}
	for (auto& t : threads)
	{
		if (t.joinable())
			t.join();
	}
	loggerTest.reset();
	auto end = std::chrono::high_resolution_clock::now();
	auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	std::cout << "\nThreads counts: " << thread_count << "\n";
	std::cout << "Logger result: " << time << "ms\n\n";
}

TEST(Logger, TenThreadsSpeedTest)
{
	int thread_count = 10;
	auto start = std::chrono::high_resolution_clock::now();
	auto file_strategy = std::make_shared<FileStrategy>(PROD);
	std::unique_ptr<ILogger> loggerTest = std::make_unique<Logger>(file_strategy);

	std::vector<std::thread> threads;
	for (size_t i = 0; i < thread_count; ++i)
	{
		threads.emplace_back(std::thread([&loggerTest](){
				int j = 0;
				std::string msg = "SpeedTest Log Message";
				while (j < 100000)
				{
					loggerTest->Log(PROD, msg);
					++j;
				}
			}));
	}
	for (auto& t : threads)
	{
		if (t.joinable())
			t.join();
	}
	loggerTest.reset();
	auto end = std::chrono::high_resolution_clock::now();
	auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	std::cout << "\nThreads counts: " << thread_count << "\n";
	std::cout << "Logger result: " << time << "ms\n\n";
}
