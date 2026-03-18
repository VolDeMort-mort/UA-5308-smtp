#pragma once

#include <memory>
#include <string>
#include <sqlite3.h>
#include "../logger/include/Logger.h"

class DataBaseManager
{
public:
    DataBaseManager(const std::string& db_path,
                    const std::string& migration_path,
                    std::shared_ptr<ILogger> logger = nullptr);
    ~DataBaseManager();

    DataBaseManager(const DataBaseManager&) = delete;
    DataBaseManager& operator=(const DataBaseManager&) = delete;
    DataBaseManager(DataBaseManager&&) = delete;
    DataBaseManager& operator=(DataBaseManager&&) = delete;

    sqlite3* getDB() const;
    bool isConnected() const;

private:
    sqlite3* m_db = nullptr;
    bool m_connected = false;
    std::shared_ptr<ILogger> m_logger;

    bool applyMigration(const std::string& migration_path);
};