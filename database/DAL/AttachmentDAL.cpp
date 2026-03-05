#include "AttachmentDAL.h"

#include <iostream>

#define ATTACHMENT_SELECT \
    "SELECT id, message_id, file_name, mime_type, file_size, storage_path, uploaded_at " \
    "FROM attachments "

AttachmentDAL::AttachmentDAL(sqlite3* db) : m_db(db) {}

bool AttachmentDAL::setError(const char* sqlite_errmsg)
{
    m_last_error = sqlite_errmsg ? sqlite_errmsg : "unknown error";
    //Log(Error, <<m_last_error);
    return false;
}

const std::string& AttachmentDAL::getLastError() const
{
    return m_last_error;
}

Attachment AttachmentDAL::rowToAttachment(sqlite3_stmt* stmt)
{
    Attachment a;

    if(sqlite3_column_type(stmt, 0) != SQLITE_NULL) a.id = sqlite3_column_int64(stmt, 0);

    a.message_id = sqlite3_column_int64(stmt, 1);

    auto text = [&](int col) -> std::string
    {
        const unsigned char* raw = sqlite3_column_text(stmt, col);
        return raw ? reinterpret_cast<const char*>(raw) : "";
    };
    a.file_name = text(2);

    if(sqlite3_column_type(stmt, 3) != SQLITE_NULL) a.mime_type = text(3);

    if(sqlite3_column_type(stmt, 4) != SQLITE_NULL) a.file_size = sqlite3_column_int64(stmt, 4);

    a.storage_path = text(5);
    a.uploaded_at = text(6);

    return a;
}

std::vector<Attachment> AttachmentDAL::fetchRows(sqlite3_stmt* stmt) const
{
    std::vector<Attachment> results;
    while(sqlite3_step(stmt) == SQLITE_ROW) results.push_back(rowToAttachment(stmt));

    sqlite3_finalize(stmt);
    return results;
}

std::optional<Attachment> AttachmentDAL::findByID(int64_t id) const
{
    const char* sql = ATTACHMENT_SELECT "WHERE id = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int64(stmt, 1, id);

    std::optional<Attachment> result;

    if(sqlite3_step(stmt) == SQLITE_ROW) result = rowToAttachment(stmt);

    sqlite3_finalize(stmt);
    return result;
}

std::vector<Attachment> AttachmentDAL::findByMessage(int64_t message_id) const
{
    const char* sql = ATTACHMENT_SELECT "WHERE message_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return {};

    sqlite3_bind_int64(stmt, 1, message_id);
    return fetchRows(stmt);
}

bool AttachmentDAL::insert(Attachment& attachment)
{
    const char* sql =
        "INSERT INTO attachments "
        "  (message_id, file_name, mime_type, file_size, storage_path) "
        "VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_db));

    sqlite3_bind_int64(stmt, 1, attachment.message_id);
    sqlite3_bind_text(stmt, 2, attachment.file_name.c_str(), -1, SQLITE_TRANSIENT);

    if (attachment.mime_type.has_value())
        sqlite3_bind_text(stmt, 3, attachment.mime_type.value().c_str(), -1, SQLITE_TRANSIENT);
    else
        sqlite3_bind_null(stmt, 3);

    if (attachment.file_size.has_value())
        sqlite3_bind_int64(stmt, 4, attachment.file_size.value());
    else
        sqlite3_bind_null(stmt, 4);

    sqlite3_bind_text(stmt, 5, attachment.storage_path.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (ok)
        attachment.id = sqlite3_last_insert_rowid(m_db);
    else
        setError(sqlite3_errmsg(m_db));

    sqlite3_finalize(stmt);
    return ok;
}

bool AttachmentDAL::update(const Attachment& attachment)
{
    if (!attachment.id.has_value())
        return setError("update() called on an Attachment with no id");

    const char* sql =
        "UPDATE attachments "
        "SET message_id   = ?, "
        "    file_name    = ?, "
        "    mime_type    = ?, "
        "    file_size    = ?, "
        "    storage_path = ? "
        "WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_db));

    sqlite3_bind_int64(stmt, 1, attachment.message_id);
    sqlite3_bind_text(stmt, 2, attachment.file_name.c_str(), -1, SQLITE_TRANSIENT);

    if (attachment.mime_type.has_value())
        sqlite3_bind_text(stmt, 3, attachment.mime_type.value().c_str(), -1, SQLITE_TRANSIENT);
    else
        sqlite3_bind_null(stmt, 3);

    if (attachment.file_size.has_value())
        sqlite3_bind_int64(stmt, 4, attachment.file_size.value());
    else
        sqlite3_bind_null(stmt, 4);

    sqlite3_bind_text(stmt, 5, attachment.storage_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 6, attachment.id.value());

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_db));

    sqlite3_finalize(stmt);
    return ok;
}

bool AttachmentDAL::hardDelete(int64_t id)
{
    const char* sql = "DELETE FROM attachments WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_db));

    sqlite3_bind_int64(stmt, 1, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_db));

    sqlite3_finalize(stmt);
    return ok;
}