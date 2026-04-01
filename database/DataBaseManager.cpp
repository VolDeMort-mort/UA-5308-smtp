#include "DataBaseManager.h"

DataBaseManager::DataBaseManager(const std::string& db_path, std::string_view migration_sql,
                                 std::shared_ptr<ILogger> logger, int read_pool_size)
    : m_logger(std::move(logger))
{
    int rc = sqlite3_open(db_path.c_str(), &m_db);
    if (rc != SQLITE_OK)
    {
        if (m_logger) m_logger->Log(LogLevel::PROD, "[DB] Failed to open: " + std::string(sqlite3_errmsg(m_db)));
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }

    if (m_logger) m_logger->Log(LogLevel::DEBUG, "[DB] Opened: " + db_path);

    rc = sqlite3_exec(m_db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK)
    {
        if (m_logger) m_logger->Log(LogLevel::PROD, "[DB] Failed to enable WAL: " + std::string(sqlite3_errmsg(m_db)));
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }

    rc = sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK)
    {
        if (m_logger) m_logger->Log(LogLevel::PROD, "[DB] Failed to enable foreign keys: " + std::string(sqlite3_errmsg(m_db)));
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }

    if (!applyMigration(migration_sql))
    {
        if (m_logger) m_logger->Log(LogLevel::PROD, "[DB] Migration failed");
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }

    if (m_logger) m_logger->Log(LogLevel::DEBUG, "[DB] Migration applied");

    try
    {
        m_read_pool = std::make_unique<ConnectionPool>(db_path, read_pool_size);
    }
    catch (const std::exception& e)
    {
        if (m_logger) m_logger->Log(LogLevel::PROD, "[DB] Pool creation failed: " + std::string(e.what()));
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }

    m_connected = true;
}

DataBaseManager::~DataBaseManager()
{
    m_read_pool.reset();
    if (m_db) sqlite3_close(m_db);
}

sqlite3* DataBaseManager::getDB() const
{
    if (!m_connected) return nullptr;
    return m_db;
}

bool DataBaseManager::isConnected() const
{
    return m_connected;
}

bool DataBaseManager::applyMigration(std::string_view migration_sql)
{
    std::string sql(migration_sql);
    char* err_msg = nullptr;
    int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK)
    {
        if (m_logger) m_logger->Log(LogLevel::PROD, "[DB] Migration error: " + std::string(err_msg ? err_msg : "unknown"));
        sqlite3_free(err_msg);
        return false;
    }

    return true;
}