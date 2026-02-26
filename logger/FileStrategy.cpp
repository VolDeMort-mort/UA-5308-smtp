#include "FileStrategy.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

FileStrategy::FileStrategy(LogLevel defaultLevel)
	: m_current_path(FILE_PATH), m_old_path(OLD_FILE_PATH), m_default_log_level(defaultLevel)
{
	if (std::filesystem::exists(m_current_path))
		m_current_file_size = std::filesystem::file_size(m_current_path);
	else
		m_current_file_size = 0;
	if (!OpenFile()) std::cerr << "Can`t open log file: " << m_current_path << "\n";
}

bool FileStrategy::Write(const std::string& message)
{
	// std::unique_lock<std::shared_mutex> lock(shared_mutex);
	if (!IsValid()) return false;

	if (!CanWrite(message.size()))
	{
		Rotate();
		if (!IsValid()) return false;
	}

	m_file << message;

	m_current_file_size += message.size();
	return true;
}
void FileStrategy::Rotate()
{
	if (m_file.is_open()) m_file.close();
	if (std::filesystem::exists(m_old_path)) // when both files are filled,delete older one
		std::filesystem::remove(m_old_path);

	if (std::filesystem::exists(m_current_path)) 
		std::filesystem::rename(m_current_path, m_old_path);

	if (!OpenFile()) std::cerr << "Can`t open log file: " << m_current_path << "\n";
	m_current_file_size = 0;
}
bool FileStrategy::CanWrite(int message_size)
{
	return (m_current_file_size + message_size) <= MAX_FILE_SIZE;
}
bool FileStrategy::OpenFile()
{
	m_file.open(m_current_path, std::ios::app);
	return IsValid();
}
bool FileStrategy::IsValid()
{
	return m_file.is_open() && !m_file.fail();
}

void FileStrategy::Flush()
{
	m_file.flush();
}
FileStrategy::~FileStrategy()
{
	m_file.close();
}

std::string FileStrategy::SpecificLog(LogLevel lvl, const std::string& msg)
{
	if (lvl > m_default_log_level) return {};

	auto now = std::chrono::system_clock::now();
	auto sec = std::chrono::system_clock::to_time_t(now);

	std::tm tm{}; // struct for date and time(from <chrono>)

#ifdef _WIN32 // on win and unix sys we need different function calls
	localtime_s(&tm, &sec);
#else
	localtime_r(&sec, &tm);
#endif

	const char* levelStr = "UNKNOWN";
	switch (lvl)
	{
	case LogLevel::NONE:
		levelStr = "NONE";
		break;
	case LogLevel::PROD:
		levelStr = "PROD";
		break;
	case LogLevel::DEBUG:
		levelStr = "DEBUG";
		break;
	case LogLevel::TRACE:
		levelStr = "TRACE";
		break;
	}

	auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());

	char buffer[512];

	int headerLen = std::snprintf(
		buffer, sizeof(buffer), "[%04d-%02d-%02d %02d:%02d:%02d] [%s] ",
		tm.tm_year + 1900, // tm struc counts ammount of years passed since 1900(thats why we need +1900 to get 2026)
		tm.tm_mon + 1,	   // months are indexes, here we convert for normal view
		tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, levelStr, tid);

	if (headerLen < 0) return {};

	size_t remaining = sizeof(buffer) - static_cast<size_t>(headerLen);
	size_t copyLen = std::min(msg.size(), remaining - 2);

	std::memcpy(buffer + headerLen, msg.data(), copyLen);

	size_t totalLen = headerLen + copyLen;
	buffer[totalLen++] = '\n';
	buffer[totalLen] = '\0';

	return std::string(buffer, totalLen);
}

std::vector<std::string> FileStrategy::Read(size_t limit)
{
	std::vector<std::string> result;
	std::ifstream file(m_current_path, std::ios::binary);

	if (!file) return result;

	file.seekg(0, std::ios::end);
	std::streamoff pos = file.tellg();

	std::string buffer;
	size_t linesFound = 0;

	while (pos > 0 && linesFound < limit)
	{
		pos--;
		file.seekg(pos);
		char c;
		file.get(c);

		if (c == '\n') linesFound++;

		buffer.push_back(c);
	}

	std::reverse(buffer.begin(), buffer.end());

	std::stringstream ss(buffer);
	std::string line;

	while (std::getline(ss, line))
		result.push_back(line);

	if (result.size() > limit) result.erase(result.begin(), result.begin() + result.size() - limit);

	return result;
}

std::vector<std::string> FileStrategy::Search(LogLevel lvl, size_t limit, int readN)
{
	std::vector<std::string> results;

	std::string levelStr;

	switch (lvl)
	{
	case LogLevel::NONE:
		levelStr = "[NONE]";
		break;
	case LogLevel::PROD:
		levelStr = "[PROD]";
		break;
	case LogLevel::DEBUG:
		levelStr = "[DEBUG]";
		break;
	case LogLevel::TRACE:
		levelStr = "[TRACE]";
		break;
	default:
		return results;
	}

	auto logs = Read(readN);

	for (const auto& log : logs)
	{
		if (log.find(levelStr) != std::string::npos)
		{
			results.push_back(log);
			if (results.size() >= limit) break;
		}
	}

	return results;
}
