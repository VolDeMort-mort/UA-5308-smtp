#include "MessageDAL.h"

#define MESSAGE_SELECT \
    "SELECT id, user_id, folder_id, uid, " \
    "       raw_file_path, size_bytes, mime_structure, " \
    "       message_id_header, in_reply_to, references_header, " \
    "       from_address, sender_address, subject, " \
    "       is_seen, is_deleted, is_draft, is_answered, is_flagged, is_recent, " \
    "       internal_date, date_header " \
    "FROM messages "

MessageDAL::MessageDAL(sqlite3* write_conn, ConnectionPool& pool)
    : m_write_conn(write_conn)
    , m_pool(pool)
{}

bool MessageDAL::setError(const char* sqlite_errmsg)
{
    m_last_error = sqlite_errmsg ? sqlite_errmsg : "unknown error";
    return false;
}

const std::string& MessageDAL::getLastError() const
{
    return m_last_error;
}

Message MessageDAL::rowToMessage(sqlite3_stmt* stmt)
{
    Message msg;

    auto text = [&](int col) -> std::string
    {
        const unsigned char* raw = sqlite3_column_text(stmt, col);
        return raw ? reinterpret_cast<const char*>(raw) : "";
    };

    auto optText = [&](int col) -> std::optional<std::string>
    {
        if (sqlite3_column_type(stmt, col) == SQLITE_NULL) return std::nullopt;
        return text(col);
    };

    if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) msg.id = sqlite3_column_int64(stmt, 0);

    msg.user_id   = sqlite3_column_int64(stmt, 1);
    msg.folder_id = sqlite3_column_int64(stmt, 2);
    msg.uid       = sqlite3_column_int64(stmt, 3);

    msg.raw_file_path  = text(4);
    msg.size_bytes     = sqlite3_column_int64(stmt, 5);
    msg.mime_structure = optText(6);

    msg.message_id_header = optText(7);
    msg.in_reply_to       = optText(8);
    msg.references_header = optText(9);
    msg.from_address      = text(10);
    msg.sender_address    = optText(11);
    msg.subject           = optText(12);

    msg.is_seen     = sqlite3_column_int(stmt, 13) != 0;
    msg.is_deleted  = sqlite3_column_int(stmt, 14) != 0;
    msg.is_draft    = sqlite3_column_int(stmt, 15) != 0;
    msg.is_answered = sqlite3_column_int(stmt, 16) != 0;
    msg.is_flagged  = sqlite3_column_int(stmt, 17) != 0;
    msg.is_recent   = sqlite3_column_int(stmt, 18) != 0;

    msg.internal_date = text(19);
    msg.date_header   = optText(20);

    return msg;
}

std::vector<Message> MessageDAL::fetchRows(sqlite3_stmt* stmt) const
{
    std::vector<Message> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(rowToMessage(stmt));
    sqlite3_finalize(stmt);
    return result;
}

std::optional<Message> MessageDAL::findByID(int64_t id) const
{
    ReadGuard g(m_pool);
    const char* sql = MESSAGE_SELECT "WHERE id = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g.db(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return std::nullopt;

    sqlite3_bind_int64(stmt, 1, id);

    std::optional<Message> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = rowToMessage(stmt);

    sqlite3_finalize(stmt);
    return result;
}

std::optional<Message> MessageDAL::findByUID(int64_t folder_id, int64_t uid) const
{
    ReadGuard g(m_pool);
    const char* sql = MESSAGE_SELECT "WHERE folder_id = ? AND uid = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g.db(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return std::nullopt;

    sqlite3_bind_int64(stmt, 1, folder_id);
    sqlite3_bind_int64(stmt, 2, uid);

    std::optional<Message> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = rowToMessage(stmt);

    sqlite3_finalize(stmt);
    return result;
}

std::vector<Message> MessageDAL::findByUser(int64_t user_id, int limit, int offset) const
{
    ReadGuard g(m_pool);
    const char* sql = MESSAGE_SELECT "WHERE user_id = ? ORDER BY internal_date DESC LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g.db(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return {};

    sqlite3_bind_int64(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, limit);
    sqlite3_bind_int(stmt, 3, offset);
    return fetchRows(stmt);
}

std::vector<Message> MessageDAL::findByFolder(int64_t folder_id, int limit, int offset) const
{
    ReadGuard g(m_pool);
    const char* sql = MESSAGE_SELECT "WHERE folder_id = ? ORDER BY uid ASC LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g.db(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return {};

    sqlite3_bind_int64(stmt, 1, folder_id);
    sqlite3_bind_int(stmt, 2, limit);
    sqlite3_bind_int(stmt, 3, offset);
    return fetchRows(stmt);
}

std::vector<Message> MessageDAL::findUnseen(int64_t folder_id, int limit, int offset) const
{
    ReadGuard g(m_pool);
    const char* sql = MESSAGE_SELECT "WHERE folder_id = ? AND is_seen = 0 ORDER BY uid ASC LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g.db(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return {};

    sqlite3_bind_int64(stmt, 1, folder_id);
    sqlite3_bind_int(stmt, 2, limit);
    sqlite3_bind_int(stmt, 3, offset);
    return fetchRows(stmt);
}

std::vector<Message> MessageDAL::findDeleted(int64_t folder_id, int limit, int offset) const
{
    ReadGuard g(m_pool);
    const char* sql = MESSAGE_SELECT "WHERE folder_id = ? AND is_deleted = 1 ORDER BY uid ASC LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g.db(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return {};

    sqlite3_bind_int64(stmt, 1, folder_id);
    sqlite3_bind_int(stmt, 2, limit);
    sqlite3_bind_int(stmt, 3, offset);
    return fetchRows(stmt);
}

std::vector<Message> MessageDAL::findFlagged(int64_t folder_id, int limit, int offset) const
{
    ReadGuard g(m_pool);
    const char* sql = MESSAGE_SELECT "WHERE folder_id = ? AND is_flagged = 1 ORDER BY uid ASC LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g.db(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return {};

    sqlite3_bind_int64(stmt, 1, folder_id);
    sqlite3_bind_int(stmt, 2, limit);
    sqlite3_bind_int(stmt, 3, offset);
    return fetchRows(stmt);
}

std::vector<Message> MessageDAL::search(int64_t user_id, const std::string& query, int limit, int offset) const
{
    ReadGuard g(m_pool);
    const char* sql = MESSAGE_SELECT
        "WHERE user_id = ? AND (subject LIKE ? OR from_address LIKE ?) "
        "ORDER BY internal_date DESC LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g.db(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return {};

    std::string pattern = "%" + query + "%";
    sqlite3_bind_int64(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, limit);
    sqlite3_bind_int(stmt, 5, offset);
    return fetchRows(stmt);
}

bool MessageDAL::insert(Message& msg)
{
    const char* sql =
        "INSERT INTO messages "
        "  (user_id, folder_id, uid, raw_file_path, size_bytes, mime_structure, "
        "   message_id_header, in_reply_to, references_header, "
        "   from_address, sender_address, subject, "
        "   is_seen, is_deleted, is_draft, is_answered, is_flagged, is_recent, "
        "   internal_date, date_header) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_write_conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_write_conn));

    auto bindOptText = [&](int col, const std::optional<std::string>& val)
    {
        if (val.has_value())
            sqlite3_bind_text(stmt, col, val->c_str(), -1, SQLITE_TRANSIENT);
        else
            sqlite3_bind_null(stmt, col);
    };

    sqlite3_bind_int64(stmt, 1, msg.user_id);
    sqlite3_bind_int64(stmt, 2, msg.folder_id);
    sqlite3_bind_int64(stmt, 3, msg.uid);
    sqlite3_bind_text(stmt, 4, msg.raw_file_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 5, msg.size_bytes);
    bindOptText(6, msg.mime_structure);
    bindOptText(7, msg.message_id_header);
    bindOptText(8, msg.in_reply_to);
    bindOptText(9, msg.references_header);
    sqlite3_bind_text(stmt, 10, msg.from_address.c_str(), -1, SQLITE_TRANSIENT);
    bindOptText(11, msg.sender_address);
    bindOptText(12, msg.subject);
    sqlite3_bind_int(stmt, 13, msg.is_seen     ? 1 : 0);
    sqlite3_bind_int(stmt, 14, msg.is_deleted  ? 1 : 0);
    sqlite3_bind_int(stmt, 15, msg.is_draft    ? 1 : 0);
    sqlite3_bind_int(stmt, 16, msg.is_answered ? 1 : 0);
    sqlite3_bind_int(stmt, 17, msg.is_flagged  ? 1 : 0);
    sqlite3_bind_int(stmt, 18, msg.is_recent   ? 1 : 0);
    sqlite3_bind_text(stmt, 19, msg.internal_date.c_str(), -1, SQLITE_TRANSIENT);
    bindOptText(20, msg.date_header);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (ok)
        msg.id = sqlite3_last_insert_rowid(m_write_conn);
    else
        setError(sqlite3_errmsg(m_write_conn));

    sqlite3_finalize(stmt);
    return ok;
}

bool MessageDAL::update(const Message& msg)
{
    if (!msg.id.has_value())
        return setError("update() called on a Message with no id");

    const char* sql =
        "UPDATE messages SET "
        "  user_id = ?, folder_id = ?, uid = ?, "
        "  raw_file_path = ?, size_bytes = ?, mime_structure = ?, "
        "  message_id_header = ?, in_reply_to = ?, references_header = ?, "
        "  from_address = ?, sender_address = ?, subject = ?, "
        "  is_seen = ?, is_deleted = ?, is_draft = ?, "
        "  is_answered = ?, is_flagged = ?, is_recent = ?, "
        "  internal_date = ?, date_header = ? "
        "WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_write_conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_write_conn));

    auto bindOptText = [&](int col, const std::optional<std::string>& val)
    {
        if (val.has_value())
            sqlite3_bind_text(stmt, col, val->c_str(), -1, SQLITE_TRANSIENT);
        else
            sqlite3_bind_null(stmt, col);
    };

    sqlite3_bind_int64(stmt, 1, msg.user_id);
    sqlite3_bind_int64(stmt, 2, msg.folder_id);
    sqlite3_bind_int64(stmt, 3, msg.uid);
    sqlite3_bind_text(stmt, 4, msg.raw_file_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 5, msg.size_bytes);
    bindOptText(6, msg.mime_structure);
    bindOptText(7, msg.message_id_header);
    bindOptText(8, msg.in_reply_to);
    bindOptText(9, msg.references_header);
    sqlite3_bind_text(stmt, 10, msg.from_address.c_str(), -1, SQLITE_TRANSIENT);
    bindOptText(11, msg.sender_address);
    bindOptText(12, msg.subject);
    sqlite3_bind_int(stmt, 13, msg.is_seen     ? 1 : 0);
    sqlite3_bind_int(stmt, 14, msg.is_deleted  ? 1 : 0);
    sqlite3_bind_int(stmt, 15, msg.is_draft    ? 1 : 0);
    sqlite3_bind_int(stmt, 16, msg.is_answered ? 1 : 0);
    sqlite3_bind_int(stmt, 17, msg.is_flagged  ? 1 : 0);
    sqlite3_bind_int(stmt, 18, msg.is_recent   ? 1 : 0);
    sqlite3_bind_text(stmt, 19, msg.internal_date.c_str(), -1, SQLITE_TRANSIENT);
    bindOptText(20, msg.date_header);
    sqlite3_bind_int64(stmt, 21, msg.id.value());

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_write_conn));

    sqlite3_finalize(stmt);
    return ok;
}

bool MessageDAL::updateSeen(int64_t id, bool seen)
{
    const char* sql = "UPDATE messages SET is_seen = ? WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_write_conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_write_conn));

    sqlite3_bind_int(stmt, 1, seen ? 1 : 0);
    sqlite3_bind_int64(stmt, 2, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_write_conn));

    sqlite3_finalize(stmt);
    return ok;
}

bool MessageDAL::updateDeleted(int64_t id, bool deleted)
{
    const char* sql = "UPDATE messages SET is_deleted = ? WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_write_conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_write_conn));

    sqlite3_bind_int(stmt, 1, deleted ? 1 : 0);
    sqlite3_bind_int64(stmt, 2, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_write_conn));

    sqlite3_finalize(stmt);
    return ok;
}

bool MessageDAL::updateFlags(int64_t id, bool is_seen, bool is_deleted, bool is_draft,
                             bool is_answered, bool is_flagged, bool is_recent)
{
    const char* sql =
        "UPDATE messages SET "
        "  is_seen = ?, is_deleted = ?, is_draft = ?, "
        "  is_answered = ?, is_flagged = ?, is_recent = ? "
        "WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_write_conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_write_conn));

    sqlite3_bind_int(stmt, 1, is_seen     ? 1 : 0);
    sqlite3_bind_int(stmt, 2, is_deleted  ? 1 : 0);
    sqlite3_bind_int(stmt, 3, is_draft    ? 1 : 0);
    sqlite3_bind_int(stmt, 4, is_answered ? 1 : 0);
    sqlite3_bind_int(stmt, 5, is_flagged  ? 1 : 0);
    sqlite3_bind_int(stmt, 6, is_recent   ? 1 : 0);
    sqlite3_bind_int64(stmt, 7, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_write_conn));

    sqlite3_finalize(stmt);
    return ok;
}

bool MessageDAL::moveToFolder(int64_t id, int64_t folder_id, int64_t new_uid)
{
    const char* sql = "UPDATE messages SET folder_id = ?, uid = ? WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_write_conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_write_conn));

    sqlite3_bind_int64(stmt, 1, folder_id);
    sqlite3_bind_int64(stmt, 2, new_uid);
    sqlite3_bind_int64(stmt, 3, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_write_conn));

    sqlite3_finalize(stmt);
    return ok;
}

bool MessageDAL::hardDelete(int64_t id)
{
    const char* sql = "DELETE FROM messages WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_write_conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_write_conn));

    sqlite3_bind_int64(stmt, 1, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_write_conn));

    sqlite3_finalize(stmt);
    return ok;
}

bool MessageDAL::clearRecentByFolder(int64_t folder_id)
{
    const char* sql = "UPDATE messages SET is_recent = 0 WHERE folder_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_write_conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_write_conn));

    sqlite3_bind_int64(stmt, 1, folder_id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_write_conn));

    sqlite3_finalize(stmt);
    return ok;
}