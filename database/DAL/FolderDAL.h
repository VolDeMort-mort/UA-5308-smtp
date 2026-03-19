#pragma once

#include <vector>
#include <optional>
#include <string>
#include <cstdint>
#include <sqlite3.h>

#include "../Entity/Folder.h"

class FolderDAL
{
public:
    explicit FolderDAL(sqlite3* db);

    std::optional<Folder> findByID(int64_t id) const;
    std::vector<Folder> findByUser(int64_t user_id, int limit = 50, int offset = 0) const;
    std::optional<Folder> findByName(int64_t user_id, const std::string& name) const;

    bool insert(Folder& folder);
    bool update(const Folder& folder);
    bool incrementNextUID(int64_t id);
    bool hardDelete(int64_t id);

    const std::string& getLastError() const;

private:
    sqlite3* m_db;
    std::string m_last_error;

    bool setError(const char* sqlite_errmsg);
    std::vector<Folder> fetchRows(sqlite3_stmt* stmt) const;
    static Folder rowToFolder(sqlite3_stmt* stmt);
};  