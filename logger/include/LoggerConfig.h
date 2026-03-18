#pragma once

#include <chrono>

namespace ISXLoggerConfig {
	inline constexpr size_t LogsForBatch = 50;
	inline constexpr std::chrono::seconds Timeout{ 2 };
	inline constexpr size_t MaxFileSize = 10 * 1024 * 1024; // 10mb || ~260k logs
	inline constexpr const char* FilePath = "log.txt";
	inline constexpr const char* OldFilePath = "old_log.txt";
};