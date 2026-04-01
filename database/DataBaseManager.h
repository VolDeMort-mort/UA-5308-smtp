#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <sqlite3.h>
#include "Logger.h"
#include "ConnectionPool.h"

class DataBaseManager
{
public:
    DataBaseManager(const std::string& db_path, std::string_view migration_sql,
                    std::shared_ptr<ILogger> logger = nullptr, int read_pool_size = 0);
    ~DataBaseManager();

    DataBaseManager(const DataBaseManager&) = delete;
    DataBaseManager& operator=(const DataBaseManager&) = delete;
    DataBaseManager(DataBaseManager&&) = delete;
    DataBaseManager& operator=(DataBaseManager&&) = delete;

    sqlite3* getDB() const;
    bool isConnected() const;

    ConnectionPool& pool() { return *m_read_pool; }
    std::unique_lock<std::mutex> writeLock() { return std::unique_lock<std::mutex>(m_write_mutex); }

private:
    sqlite3* m_db = nullptr;
    bool m_connected = false;
    std::shared_ptr<ILogger> m_logger;
    std::mutex m_write_mutex;
    std::unique_ptr<ConnectionPool> m_read_pool;

    bool applyMigration(std::string_view migration_sql);
};