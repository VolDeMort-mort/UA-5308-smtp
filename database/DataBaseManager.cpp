#include "DataBaseManager.h"

#include <fstream>
#include <sstream>

DataBaseManager::DataBaseManager(const std::string& db_path,
                                 const std::string& migration_path,
                                 std::shared_ptr<ILogger> logger)
    : m_logger(std::move(logger))
{
    int rc = sqlite3_open(db_path.c_str(), &m_db);

    if (rc != SQLITE_OK)
    {
        if (m_logger)
            m_logger->Log(LogLevel::PROD, "[DB] Failed to open: " + std::string(sqlite3_errmsg(m_db)));
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }

    if (m_logger)
        m_logger->Log(LogLevel::DEBUG, "[DB] Opened: " + db_path);

    rc = sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK)
    {
        if (m_logger)
            m_logger->Log(LogLevel::PROD, "[DB] Failed to enable foreign keys: "
                          + std::string(sqlite3_errmsg(m_db)));
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }

    if (!applyMigration(migration_path))
    {
        if (m_logger)
            m_logger->Log(LogLevel::PROD, "[DB] Migration failed: " + migration_path);
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }

    if (m_logger)
        m_logger->Log(LogLevel::DEBUG, "[DB] Migration applied: " + migration_path);

    m_connected = true;
}

DataBaseManager::~DataBaseManager()
{
    if (m_db)
        sqlite3_close(m_db);
}

sqlite3* DataBaseManager::getDB() const
{
    if (!m_connected)
        return nullptr;
    return m_db;
}

bool DataBaseManager::isConnected() const
{
    return m_connected;
}

bool DataBaseManager::applyMigration(const std::string& migration_path)
{
    std::ifstream file(migration_path);

    if (!file.is_open())
    {
        if (m_logger)
            m_logger->Log(LogLevel::PROD, "[DB] Cannot open migration file: " + migration_path);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sql = buffer.str();

    char* err_msg = nullptr;
    int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK)
    {
        if (m_logger)
            m_logger->Log(LogLevel::PROD, "[DB] Migration error: "
                          + std::string(err_msg ? err_msg : "unknown"));
        sqlite3_free(err_msg);
        return false;
    }

    return true;
}