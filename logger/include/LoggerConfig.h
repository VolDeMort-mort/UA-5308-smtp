#pragma once

#include <chrono>

namespace ISXLoggerConfig
{
inline constexpr size_t LogsForBatch = 50;
inline constexpr std::chrono::seconds Timeout{2};
inline constexpr size_t MaxFileSize = 10 * 1024 * 1024; // 10mb || ~260k logs
inline constexpr const char* FilePath = "log.txt";
inline constexpr const char* OldFilePath = "old_log.txt";

namespace Color
{
inline constexpr const char* Error = "\x1B[91m";
inline constexpr const char* Info = "\x1B[32m";

inline constexpr const char* Prod = "\x1B[94m";
inline constexpr const char* Debug = "\x1B[33m";
inline constexpr const char* Trace = "\x1B[95m";
inline constexpr const char* Reset = "\x1B[0m";
} // namespace Color
}; // namespace ISXLoggerConfig