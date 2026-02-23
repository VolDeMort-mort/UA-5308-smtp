#pragma once

#include <string>

#include "sqlite3/sqlite3.h"

class DataBaseManager
{
public:
    DataBaseManager(const std::string& db_path, const std::string& mygration_path );

    ~DataBaseManager();

    DataBaseManager(const DataBaseManager&) = delete;

    DataBaseManager& operator=(const DataBaseManager&) = delete;

    sqlite3* getDB() const;

    bool isConnected() const;

private:
    sqlite3* m_db = nullptr;
    bool m_connected  = false;

    bool applyMygration(const std::string& mygration_path);
};