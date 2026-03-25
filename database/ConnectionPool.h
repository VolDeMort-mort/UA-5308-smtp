#pragma once

#include <sqlite3.h>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <stdexcept>

#define THREAD_COUNT 8

class ConnectionPool
{
public:
    explicit ConnectionPool(const std::string& db_path, int pool_size = THREAD_COUNT);
    ~ConnectionPool();

    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

    sqlite3* acquire();
    void release(sqlite3* conn);

private:
    struct Slot{ sqlite3* conn = nullptr; bool in_use = false; };

    std::vector<Slot> m_slots;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

class ReadGuard
{
public:
    explicit ReadGuard(ConnectionPool& pool) : m_pool(pool), m_conn(pool.acquire()) {}
    ~ReadGuard() {m_pool.release(m_conn);}

    sqlite3* db() const {return m_conn;};

    ReadGuard(const ReadGuard&) = delete;
    ReadGuard& operator=(const ReadGuard&) = delete;
private:
    ConnectionPool& m_pool;
    sqlite3* m_conn;
};