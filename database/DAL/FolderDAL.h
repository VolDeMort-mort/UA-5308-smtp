#pragma once

#include <vector>
#include <optional>
#include <string>
#include <cstdint>
#include <sqlite3.h>

#include "Entity/Folder.h"
#include "ConnectionPool.h"

class FolderDAL
{
public:
    explicit FolderDAL(sqlite3* write_conn, ConnectionPool& pool);

    std::optional<Folder> findByID(int64_t id) const;
    std::vector<Folder> findByUser(int64_t user_id, int limit = 50, int offset = 0) const;
    std::optional<Folder> findByName(int64_t user_id, const std::string& name) const;
    std::vector<Folder> findByParent(int64_t parent_id, int limit = 50, int offset = 0) const;

    bool insert(Folder& folder);
    bool update(const Folder& folder);
    bool incrementNextUID(int64_t id);
    bool hardDelete(int64_t id);
    bool setSubscribed(int64_t folder_id, bool subscribed);
    

    const std::string& getLastError() const;

private:
    sqlite3* m_write_conn;
    ConnectionPool& m_pool;
    mutable std::string m_last_error;

    bool setError(const char* sqlite_errmsg);
    std::vector<Folder> fetchRows(sqlite3_stmt* stmt) const;
    static Folder rowToFolder(sqlite3_stmt* stmt);
};