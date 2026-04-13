#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <string>
#include <unistd.h>

#include "DataBaseManager.h"
#include "Logger.h"
#include "ConsoleStrategy.h"
#include "schema.h"

namespace {

std::atomic<int> g_dbm_counter{0};

std::string uniqueDbPath() {
    auto tmp = std::filesystem::temp_directory_path();
    return (tmp / ("dbmgr_" + std::to_string(getpid()) + "_" + std::to_string(++g_dbm_counter) + ".db")).string();
}

void removeDb(const std::string& path) {
    for (const char* suffix : {"", "-wal", "-shm"})
        std::filesystem::remove(path + suffix);
}

} 


TEST(DataBaseManagerTest, MemoryDatabaseConnects) {
    DataBaseManager mgr(":memory:", initSchema());
    EXPECT_TRUE(mgr.isConnected());
    EXPECT_NE(mgr.getDB(), nullptr);
}

TEST(DataBaseManagerTest, FileDatabaseConnects) {
    std::string path = uniqueDbPath();
    {
        DataBaseManager mgr(path, initSchema());
        EXPECT_TRUE(mgr.isConnected());
        EXPECT_NE(mgr.getDB(), nullptr);
    }
    removeDb(path);
}


TEST(DataBaseManagerTest, InvalidPathNotConnected) {
    DataBaseManager mgr("/nonexistent/path/that/cannot/exist/db.sqlite", initSchema());
    EXPECT_FALSE(mgr.isConnected());
    EXPECT_EQ(mgr.getDB(), nullptr);
}

TEST(DataBaseManagerTest, BadMigrationSqlNotConnected) {
    DataBaseManager mgr(":memory:", "THIS IS NOT VALID SQL;");
    EXPECT_FALSE(mgr.isConnected());
    EXPECT_EQ(mgr.getDB(), nullptr);
}

TEST(DataBaseManagerTest, GetDbReturnsNullWhenNotConnected) {
    DataBaseManager mgr("/no/such/dir/x.db", initSchema());
    EXPECT_EQ(mgr.getDB(), nullptr);
}

TEST(DataBaseManagerTest, ConnectsWithExplicitPoolSize) {
    std::string path = uniqueDbPath();
    {
        DataBaseManager mgr(path, initSchema(), nullptr, 2);
        EXPECT_TRUE(mgr.isConnected());
        EXPECT_NE(mgr.getDB(), nullptr);
    }
    removeDb(path);
}


TEST(DataBaseManagerTest, WriteLockAcquirable) {
    DataBaseManager mgr(":memory:", initSchema());
    ASSERT_TRUE(mgr.isConnected());
    {
        auto lock = mgr.writeLock();
        EXPECT_TRUE(lock.owns_lock());
    }
}

TEST(DataBaseManagerTest, WithLoggerConnects) {
    auto logger = std::make_shared<Logger>(std::make_unique<ConsoleStrategy>());
    DataBaseManager mgr(":memory:", initSchema(), logger);
    EXPECT_TRUE(mgr.isConnected());
    EXPECT_NE(mgr.getDB(), nullptr);
}

TEST(DataBaseManagerTest, WithLoggerInvalidPathNotConnected) {
    auto logger = std::make_shared<Logger>(std::make_unique<ConsoleStrategy>());
    DataBaseManager mgr("/nonexistent/path/db.sqlite", initSchema(), logger);
    EXPECT_FALSE(mgr.isConnected());
    EXPECT_EQ(mgr.getDB(), nullptr);
}

TEST(DataBaseManagerTest, WithLoggerBadMigrationNotConnected) {
    auto logger = std::make_shared<Logger>(std::make_unique<ConsoleStrategy>());
    DataBaseManager mgr(":memory:", "NOT VALID SQL;", logger);
    EXPECT_FALSE(mgr.isConnected());
    EXPECT_EQ(mgr.getDB(), nullptr);
}

TEST(DataBaseManagerTest, WithLoggerExplicitPoolSize) {
    auto logger = std::make_shared<Logger>(std::make_unique<ConsoleStrategy>());
    std::string path = uniqueDbPath();
    {
        DataBaseManager mgr(path, initSchema(), logger, 3);
        EXPECT_TRUE(mgr.isConnected());
    }
    removeDb(path);
}
