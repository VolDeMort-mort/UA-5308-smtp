#pragma once

#include "RecipientDAL.h"

#define RECIPIENT_SELECT \
    "SELECT id, message_id, address " \
    "FROM recipients "

RecipientDAL::RecipientDAL(sqlite3* db) : m_db(db) {}

bool RecipientDAL::setError(const char* sqlite_errmsg)
{
    m_last_error = sqlite_errmsg ? sqlite_errmsg : "unknown error";
    //Log(ERROR, >>m_last_error);
    return false;
}

const std::string& RecipientDAL::getLastError() const
{
    return m_last_error;
}

Recipient RecipientDAL::rowToRecipient(sqlite3_stmt* stmt)
{
    Recipient r;

    if(sqlite3_column_type(stmt, 0) != SQLITE_NULL) r.id = sqlite3_column_int64(stmt, 0);

    r.message_id = sqlite3_column_int64(stmt, 1);

    const unsigned char* raw = sqlite3_column_text(stmt, 2);
    r.address = raw ? reinterpret_cast<const char*>(raw) : "";
    
    return r;
}

std::vector<Recipient> RecipientDAL::fetchRows(sqlite3_stmt* stmt) const
{
    std::vector<Recipient> results;

    while (sqlite3_step(stmt) == SQLITE_ROW)
        results.push_back(rowToRecipient(stmt));

    sqlite3_finalize(stmt);
    return results;
}

std::optional<Recipient> RecipientDAL::findByID(int64_t id) const
{
    const char* sql = RECIPIENT_SELECT "WHERE id = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return std::nullopt;

    sqlite3_bind_int64(stmt, 1, id);

    std::optional<Recipient> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = rowToRecipient(stmt);

    sqlite3_finalize(stmt);
    return result;
}

std::vector<Recipient> RecipientDAL::findByMessage(int64_t message_id) const
{
    const char* sql = RECIPIENT_SELECT "WHERE message_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return {};

    sqlite3_bind_int64(stmt, 1, message_id);
    return fetchRows(stmt);
}

bool RecipientDAL::insert(Recipient& recipient)
{
    const char* sql =
        "INSERT INTO recipients (message_id, address) "
        "VALUES (?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_db));

    sqlite3_bind_int64(stmt, 1, recipient.message_id);
    sqlite3_bind_text(stmt, 2, recipient.address.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (ok)
        recipient.id = sqlite3_last_insert_rowid(m_db);
    else
        setError(sqlite3_errmsg(m_db));

    sqlite3_finalize(stmt);
    return ok;
}

bool RecipientDAL::update(const Recipient& recipient)
{
    if (!recipient.id.has_value())
        return setError("update() called on a Recipient with no id");

    const char* sql =
        "UPDATE recipients "
        "SET message_id = ?, address = ? "
        "WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_db));

    sqlite3_bind_int64(stmt, 1, recipient.message_id);
    sqlite3_bind_text(stmt, 2, recipient.address.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, recipient.id.value());

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_db));

    sqlite3_finalize(stmt);
    return ok;
}

bool RecipientDAL::hardDelete(int64_t id)
{
    const char* sql = "DELETE FROM recipients WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_db));

    sqlite3_bind_int64(stmt, 1, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_db));

    sqlite3_finalize(stmt);
    return ok;
}
