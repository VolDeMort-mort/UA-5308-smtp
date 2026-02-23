#include "FileStrategy.h"
#include <fstream>
#include <string>
#include <memory>

FileStrategy::FileStrategy(const std::string& path, LogLevel defaultLevel) : m_currentPath(path), m_defaultLogLevel(defaultLevel)
{
    OpenFile();
    m_workThread = std::thread(&FileStrategy::WorkQueue, this);
}

void FileStrategy::Write(const std::string& message)
{
	m_file << message;
}
bool FileStrategy::OpenFile()
{
	m_file.open(m_currentPath, std::ios::app);
	return OpenFileCheck();
}
bool FileStrategy::OpenFileCheck()
{
	return m_file.is_open();
}

void FileStrategy::PushToQueue(const std::string& message)
{
    {
        std::lock_guard<std::mutex>lock(m_queueMtx);
        m_queue.push(message);
    }
    if(m_queue.size() >= 50)
      m_cv.notify_one();
};

void FileStrategy::WorkQueue()
{
    std::queue<std::string>local_queue;
    while (runningFlag)
    {
        {
            std::unique_lock<std::mutex>lock(m_queueMtx);
            m_cv.wait_for(lock,std::chrono::seconds(2), [this] {return m_queue.size() >= 50; }); //wait 2 seconds to log data if size<50
            if (m_queue.empty() && runningFlag)
                continue;
            if (m_queue.empty() && !runningFlag)
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
    runningFlag = false;
    m_cv.notify_all();
    if (m_workThread.joinable())
        m_workThread.join();
    
	m_file.close();
}
