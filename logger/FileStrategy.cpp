#include <cstring>
#include <sstream>

#include "FileStrategy.h"

FileStrategy::FileStrategy(const std::string& path, LogLevel defaultLevel)
    : m_current_path(path), m_default_log_level(defaultLevel)
{
    OpenFile();
    m_work_thread = std::thread(&FileStrategy::WorkQueue, this);
}

void FileStrategy::Write(const std::string& message)
{
    m_file << message;
}

bool FileStrategy::OpenFile()
{
    m_file.open(m_current_path, std::ios::app);
    return OpenFileCheck();
}

bool FileStrategy::OpenFileCheck()
{
    return m_file.is_open();
}

void FileStrategy::PushToQueue(const std::string& message)
{
    {
        std::lock_guard<std::mutex>lock(m_queue_mtx);
        m_queue.push(message);
    }
    if(m_queue.size() >= 50)
      m_cv.notify_one();
};

void FileStrategy::WorkQueue()
{
    std::queue<std::string>local_queue;
    while (m_running_flag)
    {
        {
            std::unique_lock<std::mutex>lock(m_queue_mtx);
            m_cv.wait_for(lock,std::chrono::seconds(2), [this] {return m_queue.size() >= 50; }); //wait 2 seconds to log data if size<50
            if (m_queue.empty() && m_running_flag)
                continue;
            if (m_queue.empty() && !m_running_flag)
                break;

           std::swap(local_queue, m_queue);
         
        }
        if (!local_queue.empty()) {
            if(!OpenFileCheck())
            {
                //error
            }
            while (!local_queue.empty()) {
                Write(local_queue.front());
                local_queue.pop();
            }
        }
        m_file.flush();
    }
};

FileStrategy::~FileStrategy()
{
    m_running_flag = false;
    m_cv.notify_all();
    if (m_work_thread.joinable())
        m_work_thread.join();

    m_file.close();
}

void FileStrategy::SpecificLog(LogLevel lvl, const std::string& msg)
{
    if (lvl > m_default_log_level) return;

    auto now = std::chrono::system_clock::now();
    auto sec = std::chrono::system_clock::to_time_t(now);

    std::tm tm{}; // struct for date and time

    #ifdef _WIN32
        localtime_s(&tm, &sec);
    #else
        localtime_r(&sec, &tm);
    #endif

    const char* levelStr = "UNKNOWN";
    switch (lvl)
    {
        case LogLevel::NONE:  levelStr = "NONE";  break;
        case LogLevel::PROD:  levelStr = "PROD";  break;
        case LogLevel::DEBUG: levelStr = "DEBUG"; break;
        case LogLevel::TRACE: levelStr = "TRACE"; break;
    }

    auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());

    char buffer[512];

    int headerLen = std::snprintf(
        buffer, sizeof(buffer),
        "[%04d-%02d-%02d %02d:%02d:%02d] [%s] [%zu]",
        tm.tm_year + 1900, // tm counts years since 1900
        tm.tm_mon + 1,     // months are 0-indexed
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec,
        levelStr,
        tid
    );

    if (headerLen < 0) return;

    size_t remaining = sizeof(buffer) - static_cast<size_t>(headerLen);
    size_t copyLen = std::min(msg.size(), remaining - 2);

    std::memcpy(buffer + headerLen, msg.data(), copyLen);

    size_t totalLen = headerLen + copyLen;
    buffer[totalLen++] = '\n';
    buffer[totalLen]   = '\0';

    PushToQueue(std::string(buffer, totalLen));
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

    while (std::getline(ss, line)) result.push_back(line);

    if (result.size() > limit)
        result.erase(result.begin(), result.begin() + result.size() - limit);

    return result;
}

std::vector<std::string> FileStrategy::Search(LogLevel lvl, size_t limit, int readN)
{
    std::vector<std::string> results;

    std::string levelStr;

    switch (lvl)
    {
        case LogLevel::NONE:  levelStr = "[NONE]";  break;
        case LogLevel::PROD:  levelStr = "[PROD]";  break;
        case LogLevel::DEBUG: levelStr = "[DEBUG]"; break;
        case LogLevel::TRACE: levelStr = "[TRACE]"; break;
        default: return results;
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