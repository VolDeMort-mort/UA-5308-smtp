#include "MessageDAL.h"

#include <iostream>

#define BASE_SELECT \
    "SELECT id, user_id, folder_id, subject, body, receiver, " \
    "       status, is_seen, is_starred, is_important, created_at " \
    "FROM messages "

MessageDAL::MessageDAL(sqlite3* db) : m_db(db) {}

//Read

bool MessageDAL::setError(const char* sqlite_errmsg)
{
    m_last_error = sqlite_errmsg ? sqlite_errmsg : "unknown error";
    //Log(Error, "<<m_last_error");
    return false;
}

const std::string& MessageDAL::getLastError() const
{
    return m_last_error;
} 

Message MessageDAL::rowToMessage(sqlite3_stmt* stmt)
{
    Message msg;

    if(sqlite3_column_type(stmt, 0) != SQLITE_NULL) msg.id = sqlite3_column_int64(stmt, 0);

    msg.user_id = sqlite3_column_int64(stmt, 1);

    if(sqlite3_column_type(stmt, 2) != SQLITE_NULL) msg.folder_id = sqlite3_column_int64(stmt, 2);

    auto text = [&](int col) -> std::string
    {
        const unsigned char* raw = sqlite3_column_text(stmt, col);
        return raw ? reinterpret_cast<const char*>(raw) : "";
    };

    msg.subject = text(3);
    msg.body = text(4);
    msg.receiver = text(5);
    msg.status = msg.statusFromString(text(6));
    msg.is_seen = sqlite3_column_int(stmt, 7) != 0;
    msg.is_starred = sqlite3_column_int(stmt, 8) != 0;
    msg.is_important = sqlite3_column_int(stmt, 9) != 0;
    msg.created_at = text(10);

    return msg;
}

std::vector<Message> MessageDAL::fetchRows(sqlite3_stmt* stmt) const
{
    std::vector<Message> result;

    while(sqlite3_step(stmt) == SQLITE_ROW) result.push_back(rowToMessage(stmt));

    sqlite3_finalize(stmt);

    return result;
}

std::optional<Message> MessageDAL::findByID(int64_t id) const
{
    const char* sql = BASE_SELECT "WHERE id = ? LIMIT 1;";
    
    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int64(stmt, 1, id);

    std::optional<Message> result;
    if(sqlite3_step(stmt) == SQLITE_ROW) result = rowToMessage(stmt);

    sqlite3_finalize(stmt);
    return result;
}

std::vector<Message> MessageDAL::findByUser(int64_t user_id) const
{
    const char* sql = BASE_SELECT
        "WHERE user_id = ? "
        "ORDER BY created_at DESC;";
    
    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return {};

    sqlite3_bind_int64(stmt, 1, user_id);
    return fetchRows(stmt);
}

std::vector<Message> MessageDAL::findByFolder(int64_t folder_id) const
{
    const char* sql = BASE_SELECT
        "WHERE folder_id = ? "
        "ORDER BY created_at DESC;";

    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return {};

    sqlite3_bind_int64(stmt, 1, folder_id);
    return fetchRows(stmt);
}

std::vector<Message> MessageDAL::findByStatus(int64_t user_id, MessageStatus status) const
{
    const char* sql = BASE_SELECT
        "WHERE user_id = ? AND status = ? "
        "ORDER BY created_at DESC;";

    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return {};

    sqlite3_bind_int64(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, Message::statusToString(status).c_str(), -1, SQLITE_TRANSIENT);
    return fetchRows(stmt);
}

std::vector<Message> MessageDAL::findUnseen(int64_t user_id) const
{
    const char* sql = BASE_SELECT
        "WHERE user_id = ? AND is_seen = 0 "
        "ORDER BY created_at DESC;";

    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return{};

    sqlite3_bind_int64(stmt, 1, user_id);
    
    return fetchRows(stmt);
}

std::vector<Message> MessageDAL::findStarred(int64_t user_id) const
{
    const char* sql = BASE_SELECT
        "WHERE user_id = ? AND is_starred = 1 "
        "ORDER BY created_at DESC;";

    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return {}; 

    sqlite3_bind_int64(stmt, 1, user_id);
    return fetchRows(stmt);
}

std::vector<Message> MessageDAL::findImportant(int64_t user_id) const
{
    const char* sql = BASE_SELECT
        "WHERE user_id = ? AND is_important = 1 "
        "ORDER BY created_at DESC;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return {};

    sqlite3_bind_int64(stmt, 1, user_id);
    return fetchRows(stmt);
}

std::vector<Message> MessageDAL::search(int64_t user_id, const std::string& query) const
{
    const char* sql = BASE_SELECT
        "WHERE user_id = ? "
        "  AND (subject LIKE ? OR body LIKE ?) "
        "ORDER BY created_at DESC;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return {};

    std::string pattern = "%" + query + "%";
    sqlite3_bind_int64(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, pattern.c_str(), -1, SQLITE_TRANSIENT);
    
    return fetchRows(stmt);
}

//Write

bool MessageDAL::insert(Message& msg)
{
    const char* sql = 
        "INSERT INTO messages "
        "  (user_id, folder_id, subject, body, receiver, status,"
        "   is_seen, is_starred, is_important) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) 
    {
        return setError(sqlite3_errmsg(m_db));
    }

    sqlite3_bind_int64(stmt, 1, msg.user_id);
    if(msg.folder_id.has_value()) sqlite3_bind_int64(stmt, 2, msg.folder_id.value());
    else sqlite3_bind_null(stmt, 2);

    sqlite3_bind_text(stmt, 3, msg.subject.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, msg.body.c_str(),     -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, msg.receiver.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, Message::statusToString(msg.status).c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, msg.is_seen      ? 1 : 0);
    sqlite3_bind_int(stmt, 8, msg.is_starred   ? 1 : 0);
    sqlite3_bind_int(stmt, 9, msg.is_important ? 1 : 0);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if(ok) msg.id = sqlite3_last_insert_rowid(m_db);
    else setError(sqlite3_errmsg(m_db));

    sqlite3_finalize(stmt);
    return ok;
}

bool MessageDAL::update(const Message& msg)
{
    if(!msg.id.has_value()) return setError("update() on a message with no id");

    const char* sql =
        "UPDATE messages "
        "SET user_id     = ?, "
        "    folder_id   = ?, "
        "    subject     = ?, "
        "    body        = ?, "
        "    receiver    = ?, "
        "    status      = ?, "
        "    is_seen     = ?, "
        "    is_starred  = ?, "
        "    is_important = ? "
        "WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) 
    {
        return setError(sqlite3_errmsg(m_db));
    }

    sqlite3_bind_int64(stmt, 1, msg.user_id);
    if (msg.folder_id.has_value())
        sqlite3_bind_int64(stmt, 2, msg.folder_id.value());
    else
        sqlite3_bind_null(stmt, 2);

    sqlite3_bind_text(stmt, 3, msg.subject.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, msg.body.c_str(),     -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, msg.receiver.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, Message::statusToString(msg.status).c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, msg.is_seen ? 1 : 0);
    sqlite3_bind_int(stmt, 8, msg.is_starred ? 1 : 0);
    sqlite3_bind_int(stmt, 9, msg.is_important ? 1 : 0);
    sqlite3_bind_int64(stmt, 10, msg.id.value());

    
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_db));

    sqlite3_finalize(stmt);
    return ok;
}

bool MessageDAL::updateStatus(int64_t id, MessageStatus status)
{
    const char* sql = "UPDATE messages SET status = ? WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_db));

    sqlite3_bind_text(stmt, 1, Message::statusToString(status).c_str(),
                               -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_db));

    sqlite3_finalize(stmt);
    return ok;
}

bool MessageDAL::updateFlags(int64_t id, bool is_seen, bool is_starred, bool is_important)
{
    const char* sql =
        "UPDATE messages "
        "SET is_seen = ?, is_starred = ?, is_important = ? "
        "WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_db));

    sqlite3_bind_int  (stmt, 1, is_seen      ? 1 : 0);
    sqlite3_bind_int  (stmt, 2, is_starred   ? 1 : 0);
    sqlite3_bind_int  (stmt, 3, is_important ? 1 : 0);
    sqlite3_bind_int64(stmt, 4, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_db));

    sqlite3_finalize(stmt);
    return ok;
}

bool MessageDAL::moveToFolder(int64_t id, std::optional<int64_t> folder_id)
{
    const char* sql = "UPDATE messages SET folder_id = ? WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_db));

    if (folder_id.has_value())
        sqlite3_bind_int64(stmt, 1, folder_id.value());
    else
        sqlite3_bind_null(stmt, 1);

    sqlite3_bind_int64(stmt, 2, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_db));

    sqlite3_finalize(stmt);
    return ok;
}

bool MessageDAL::softDelete(int64_t id)
{
    const char* sql = "UPDATE messages SET status = 'deleted' WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_db));

    sqlite3_bind_int64(stmt, 1, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_db));

    sqlite3_finalize(stmt);
    return ok;
}

bool MessageDAL::hardDelete(int64_t id)
{
    const char* sql = "DELETE FROM messages WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_db));

    sqlite3_bind_int64(stmt, 1, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_db));

    sqlite3_finalize(stmt);
    return ok;
}

bool MessageDAL::updateSeen(int64_t id, bool seen)
{
    const char* sql = "UPDATE messages SET is_seen = ? WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_db));

    sqlite3_bind_int  (stmt, 1, seen ? 1 : 0);
    sqlite3_bind_int64(stmt, 2, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_db));

    sqlite3_finalize(stmt);
    return ok;
}