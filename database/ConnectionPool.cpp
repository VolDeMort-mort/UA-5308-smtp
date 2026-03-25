#include "ConnectionPool.h"

ConnectionPool::ConnectionPool(const std::string& db_path, int pool_size)
{
    m_slots.resize(pool_size);
    for(auto& slot : m_slots)
    {
        int rc = sqlite3_open_v2(db_path.c_str(), &slot.conn, SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX, nullptr);
        if(rc != SQLITE_OK)
        {
            for (auto& s : m_slots) if(s.conn){sqlite3_close(s.conn); s.conn = nullptr;}
            //Logger
        }
        sqlite3_exec(slot.conn, "PRAGMA journal_mode=WAL;",   nullptr, nullptr, nullptr);
        sqlite3_exec(slot.conn, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);
        sqlite3_exec(slot.conn, "PRAGMA cache_size=-4000;",   nullptr, nullptr, nullptr); // 4 MB
    }
}

ConnectionPool::~ConnectionPool()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for(auto& slot : m_slots) if (slot.conn){ sqlite3_close(slot.conn); slot.conn = nullptr;}
}

sqlite3* ConnectionPool::acquire()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [this]
    {
        for(const auto& s : m_slots) if (!s.in_use) return true;
        return false;
    });

    for (auto& slot : m_slots) if (!slot.in_use) {slot.in_use = true; return slot.conn;}

    //logger
}

void ConnectionPool::release(sqlite3* conn)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& slot : m_slots) if (slot.conn == conn) {slot.in_use = false; break;}
    }
    m_cv.notify_one();
}