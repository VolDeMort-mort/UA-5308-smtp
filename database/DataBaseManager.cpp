#include "DataBaseManager.h"

#include <iostream>
#include <fstream>
#include <sstream>

DataBaseManager::DataBaseManager(const std::string& db_path, const std::string& mygration_path)
{
    int result_code = sqlite3_open(db_path.c_str(), &m_db);

    if(result_code != SQLITE_OK)
    {
        //Log(ERROR, "failed to open database");
        std::cerr << "failed to open database";
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }

    sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

    if(!applyMygration(mygration_path))
    {
        //Log(WARNING, "Mygration Failed");
        std::cerr << "mygration failed";
        return;
    }

    m_connected = true;

    //Logger(TRACE, "Database connected");
    std::cerr << "Database connected";
}

DataBaseManager::~DataBaseManager()
{
    if(m_db)
    {
        sqlite3_close(m_db);
        //Log("Tracce", "Database closed");
        std::cerr << "Database closed";
    }
}

sqlite3* DataBaseManager::getDB() const
{
    return m_db;
}

bool DataBaseManager::isConnected() const
{
    return m_db;
}

bool DataBaseManager::applyMygration(const std::string& mygration_path)
{
    std::ifstream file(mygration_path);

    if(!file.is_open())
    {
        //Log(TRACE, "Couldn't open database mygration target");
        std::cerr << "Couldnt open db mygration target";
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sql = buffer.str();

    char* err_msg = nullptr;

    int result_code = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &err_msg);

    if(result_code != SQLITE_OK)
    {
        //Log + err msg;
        std::cerr << "mygration failed" << err_msg;
        sqlite3_free(err_msg);
        return false;
    }

    //Log(TRACE, "Mygration success");
    std::cerr << "mygration success";
    return true;
}