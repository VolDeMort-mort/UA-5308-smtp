#pragma once

#include <vector>
#include <optional>
#include <string>
#include <cstdint>

#include "../Entity/Recipient.h"
#include "../sqlite3/sqlite3.h"

class RecipientDAL
{
public:
    explicit RecipientDAL(sqlite3* db);

    std::optional<Recipient> findByID(int64_t id) const;
    std::vector<Recipient> findByMessage(int64_t message_id) const;
    bool insert(Recipient& recipient);
    bool update(const Recipient& recipient);
    bool hardDelete(int64_t id);
    
    const std::string& getLastError() const;

private:
    sqlite3* m_db;
    std::string m_last_error;

    bool setError(const char* sqlite_errmsg);

    std::vector<Recipient> fetchRows(sqlite3_stmt* stmt) const;
    static Recipient rowToRecipient(sqlite3_stmt* stmt);
};