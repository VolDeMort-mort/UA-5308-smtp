#pragma once

#include <vector>

#include "../Entity/Message.h"
#include "../sqlite3/sqlite3.h"

class MessageDAL
{
public:
    explicit MessageDAL(sqlite3* db);

    std::optional<Message> findByID(int64_t id) const;
    std::vector<Message> findByUser(int64_t user_id) const;
    std::vector<Message> findByFolder(int64_t folder_id) const;
    std::vector<Message> findByStatus(int64_t user_id, MessageStatus status) const;
    std::vector<Message> findUnseen(int64_t user_id) const;
    std::vector<Message> findStarred(int64_t user_id) const;
    std::vector<Message> findImportant(int64_t user_id) const;
    std::vector<Message> search(int64_t user_id, const std::string& query) const;

    bool insert(Message& msg);
    bool update(const Message& msg);
    bool updateSeen(int64_t id, bool seen);
    bool updateStatus(int64_t id, MessageStatus status);
    bool updateFlags(int64_t id, bool is_seen, bool is_starred, bool is_important);

    bool moveToFolder(int64_t id, std::optional<int64_t> folder_id);

    bool softDelete(int64_t id);
    bool hardDelete(int64_t id);

    const std::string& getLastError() const;
private:
    sqlite3* m_db;
    std::string m_last_error;

    bool setError(const char* sqlite_errmsg);
    std::vector<Message> fetchRows(sqlite3_stmt* stmt) const;
    static Message rowToMessage(sqlite3_stmt* stmt);
};