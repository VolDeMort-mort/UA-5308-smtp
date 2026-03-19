#pragma once

#include <vector>
#include <optional>
#include <string>
#include <cstdint>
#include <sqlite3.h>

#include "../Entity/Message.h"

class MessageDAL
{
public:
    explicit MessageDAL(sqlite3* db);

    std::optional<Message> findByID(int64_t id) const;
    std::optional<Message> findByUID(int64_t folder_id, int64_t uid) const;
    std::vector<Message> findByUser(int64_t user_id, int limit = 50, int offset = 0) const;
    std::vector<Message> findByFolder(int64_t folder_id, int limit = 50, int offset = 0) const;
    std::vector<Message> findUnseen(int64_t folder_id, int limit = 50, int offset = 0) const;
    std::vector<Message> findDeleted(int64_t folder_id, int limit = 50, int offset = 0) const;
    std::vector<Message> findFlagged(int64_t folder_id, int limit = 50, int offset = 0) const;
    std::vector<Message> search(int64_t user_id, const std::string& query, int limit = 50, int offset = 0) const;

    bool insert(Message& msg);
    bool update(const Message& msg);
    bool updateSeen(int64_t id, bool seen);
    bool updateDeleted(int64_t id, bool deleted);
    bool updateFlags(int64_t id, bool is_seen, bool is_deleted, bool is_draft,
                     bool is_answered, bool is_flagged, bool is_recent);
    bool moveToFolder(int64_t id, int64_t folder_id, int64_t new_uid);
    bool hardDelete(int64_t id);

    const std::string& getLastError() const;

private:
    sqlite3* m_db;
    std::string m_last_error;

    bool setError(const char* sqlite_errmsg);
    std::vector<Message> fetchRows(sqlite3_stmt* stmt) const;
    static Message rowToMessage(sqlite3_stmt* stmt);
};