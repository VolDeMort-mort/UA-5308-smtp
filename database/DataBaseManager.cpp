#include "DataBaseManager.h"

#include <iostream>
#include <fstream>
#include <sstream>

DataBaseManager::DataBaseManager(const std::string& db_path,
                                 const std::string& migration_path)
{
    int rc = sqlite3_open(db_path.c_str(), &m_db);

    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to open database: " << sqlite3_errmsg(m_db) << "\n";
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }

    rc = sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to enable foreign keys: " << sqlite3_errmsg(m_db) << "\n";
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }

    if (!applyMigration(migration_path))
    {
        std::cerr << "Migration failed\n";
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }

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
        std::cerr << "Could not open migration file: " << migration_path << "\n";
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sql = buffer.str();

    char* err_msg = nullptr;
    int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK)
    {
        std::cerr << "Migration failed: " << (err_msg ? err_msg : "unknown error") << "\n";
        sqlite3_free(err_msg);
        return false;
    }

    return true;
}