#include "ConnectionPool.h"
#include "../config/include/Config.h"

ConnectionPool::ConnectionPool(const std::string& db_path, int pool_size)
{
    if (pool_size <= 0)
        pool_size = SmtpClient::Config::Instance().GetServer().worker_threads;

    m_slots.resize(pool_size);
    for (auto& slot : m_slots)
    {
        int rc = sqlite3_open_v2(db_path.c_str(), &slot.conn, SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX, nullptr);
        if (rc != SQLITE_OK)
        {
            for (auto& s : m_slots)
                if (s.conn) { sqlite3_close(s.conn); s.conn = nullptr; }
            throw std::runtime_error(std::string("ConnectionPool: cannot open read connection: ") + sqlite3_errstr(rc));
        }

        sqlite3_exec(slot.conn, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
        sqlite3_exec(slot.conn, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);
        sqlite3_exec(slot.conn, "PRAGMA cache_size=-4000;", nullptr, nullptr, nullptr);
    }
}

ConnectionPool::~ConnectionPool()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& slot : m_slots)
        if (slot.conn) { sqlite3_close(slot.conn); slot.conn = nullptr; }
}

sqlite3* ConnectionPool::acquire()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [this]
    {
        for (const auto& s : m_slots) if (!s.in_use) return true;
        return false;
    });

    for (auto& slot : m_slots)
        if (!slot.in_use) { slot.in_use = true; return slot.conn; }

    throw std::runtime_error("ConnectionPool::acquire: logic error");
}

void ConnectionPool::release(sqlite3* conn)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& slot : m_slots)
            if (slot.conn == conn) { slot.in_use = false; break; }
    }
    m_cv.notify_one();
}