#pragma once

#include <vector> 
#include <optional>
#include <string>
#include <cstdint>

#include "../Entity/Attachment.h"
#include "../sqlite3/sqlite3.h"

class AttachmentDAL
{
public:
    explicit AttachmentDAL(sqlite3* db);

    std::optional<Attachment> findByID(int64_t id) const;
    std::vector<Attachment> findByMessage(int64_t message_id) const;

    bool insert(Attachment& attachment);
    bool update(const Attachment& attachment);
    bool hardDelete(int64_t id);

    const std::string& getLastError() const;

private:
    sqlite3* m_db;
    std::string m_last_error;

    bool setError(const char* sqlite_errmsg);

    std::vector<Attachment> fetchRows(sqlite3_stmt* stmt) const;
    static Attachment rowToAttachment(sqlite3_stmt* stmt);
};