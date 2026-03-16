#pragma once

#include <gtest/gtest.h>
#include <sqlite3.h>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>

// Reads the schema SQL from the path injected by CMake and applies it to db.
inline void applySchema(sqlite3* db)
{
    std::ifstream f(SCHEMA_PATH);
    if (!f.is_open())
        throw std::runtime_error("Cannot open schema: " SCHEMA_PATH);

    std::stringstream ss;
    ss << f.rdbuf();
    std::string sql = ss.str();

    char* err = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK)
    {
        std::string msg = err ? err : "unknown";
        sqlite3_free(err);
        throw std::runtime_error("Schema apply failed: " + msg);
    }
}

// Base fixture — each test gets a fresh in-memory database with the schema applied.
class DBFixture : public ::testing::Test
{
protected:
    sqlite3* db = nullptr;

    void SetUp() override
    {
        ASSERT_EQ(sqlite3_open(":memory:", &db), SQLITE_OK);
        sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
        applySchema(db);
    }

    void TearDown() override
    {
        if (db) sqlite3_close(db);
    }
};