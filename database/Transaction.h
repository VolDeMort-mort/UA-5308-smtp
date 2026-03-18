#pragma once

#include <sqlite3.h>

class Transaction
{
public:
    explicit Transaction(sqlite3* db) : m_db(db), m_committed(false), m_valid(false)
    {
        m_valid = (sqlite3_exec(m_db, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr) == SQLITE_OK);
    }

    ~Transaction()
    {
        if (m_valid && !m_committed)
            sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
    }

    bool commit()
    {
        if (!m_valid) return false;
        if (sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK)
            return false;
        m_committed = true;
        return true;
    }


    bool valid() const { return m_valid; }

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

private:
    sqlite3* m_db;
    bool     m_committed;
    bool     m_valid;
};