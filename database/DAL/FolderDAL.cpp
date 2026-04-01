#include "FolderDAL.h"

#define FOLDER_SELECT \
    "SELECT id, user_id, parent_id, name, next_uid, is_subscribed " \
    "FROM folders "

FolderDAL::FolderDAL(sqlite3* write_conn, ConnectionPool& pool)
    : m_write_conn(write_conn)
    , m_pool(pool)
{}

bool FolderDAL::setError(const char* sqlite_errmsg)
{
    m_last_error = sqlite_errmsg ? sqlite_errmsg : "unknown error";
    return false;
}

const std::string& FolderDAL::getLastError() const
{
    return m_last_error;
}

Folder FolderDAL::rowToFolder(sqlite3_stmt* stmt)
{
    Folder f;

    if (sqlite3_column_type(stmt, 0) != SQLITE_NULL)
        f.id = sqlite3_column_int64(stmt, 0);

    f.user_id = sqlite3_column_int64(stmt, 1);

    if (sqlite3_column_type(stmt, 2) != SQLITE_NULL)
    f.parent_id = sqlite3_column_int64(stmt, 2);

    const unsigned char* raw = sqlite3_column_text(stmt, 3);
    f.name = raw ? reinterpret_cast<const char*>(raw) : "";

    f.next_uid = sqlite3_column_int64(stmt, 4);
    f.is_subscribed = sqlite3_column_int(stmt, 5) != 0;

    return f;
}

std::vector<Folder> FolderDAL::fetchRows(sqlite3_stmt* stmt) const
{
    std::vector<Folder> results;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        results.push_back(rowToFolder(stmt));
    sqlite3_finalize(stmt);
    return results;
}

std::optional<Folder> FolderDAL::findByID(int64_t id) const
{
    ReadGuard g(m_pool);
    const char* sql = FOLDER_SELECT "WHERE id = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g.db(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return std::nullopt;

    sqlite3_bind_int64(stmt, 1, id);

    std::optional<Folder> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = rowToFolder(stmt);

    sqlite3_finalize(stmt);
    return result;
}

std::vector<Folder> FolderDAL::findByUser(int64_t user_id, int limit, int offset) const
{
    ReadGuard g(m_pool);
    const char* sql = FOLDER_SELECT "WHERE user_id = ? ORDER BY name ASC LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g.db(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return {};

    sqlite3_bind_int64(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, limit);
    sqlite3_bind_int(stmt, 3, offset);
    return fetchRows(stmt);
}

std::optional<Folder> FolderDAL::findByName(int64_t user_id, const std::string& name) const
{
    ReadGuard g(m_pool);
    const char* sql = FOLDER_SELECT "WHERE user_id = ? AND name = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g.db(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return std::nullopt;

    sqlite3_bind_int64(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<Folder> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = rowToFolder(stmt);

    sqlite3_finalize(stmt);
    return result;
}

bool FolderDAL::insert(Folder& folder)
{
    const char* sql =
        "INSERT INTO folders (user_id, parent_id, name, next_uid, is_subscribed) "
        "VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_write_conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_write_conn));

    sqlite3_bind_int64(stmt, 1, folder.user_id);

    if (folder.parent_id.has_value())
        sqlite3_bind_int64(stmt, 2, folder.parent_id.value());
    else
        sqlite3_bind_null(stmt, 2);

    sqlite3_bind_text(stmt, 3, folder.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, folder.next_uid);
    sqlite3_bind_int(stmt, 5, folder.is_subscribed ? 1 : 0);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (ok)
        folder.id = sqlite3_last_insert_rowid(m_write_conn);
    else
        setError(sqlite3_errmsg(m_write_conn));

    sqlite3_finalize(stmt);
    return ok;
}

bool FolderDAL::update(const Folder& folder)
{
    if (!folder.id.has_value())
        return setError("update() called on a Folder with no id");

    const char* sql =
        "UPDATE folders SET "
        "user_id = ?, parent_id = ?, name = ?, next_uid = ?, is_subscribed = ? "
        "WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_write_conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_write_conn));

    sqlite3_bind_int64(stmt, 1, folder.user_id);

    if (folder.parent_id.has_value())
        sqlite3_bind_int64(stmt, 2, folder.parent_id.value());
    else
        sqlite3_bind_null(stmt, 2);

    sqlite3_bind_text(stmt, 3, folder.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, folder.next_uid);
    sqlite3_bind_int(stmt, 5, folder.is_subscribed ? 1 : 0);
    sqlite3_bind_int64(stmt, 6, folder.id.value());

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_write_conn));

    sqlite3_finalize(stmt);
    return ok;
}

bool FolderDAL::incrementNextUID(int64_t id)
{
    const char* sql = "UPDATE folders SET next_uid = next_uid + 1 WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_write_conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_write_conn));

    sqlite3_bind_int64(stmt, 1, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_write_conn));

    sqlite3_finalize(stmt);
    return ok;
}

bool FolderDAL::hardDelete(int64_t id)
{
    const char* sql = "DELETE FROM folders WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_write_conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_write_conn));

    sqlite3_bind_int64(stmt, 1, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_write_conn));

    sqlite3_finalize(stmt);
    return ok;
}

bool FolderDAL::setSubscribed(int64_t folder_id, bool subscribed)
{
    const char* sql = "UPDATE folders SET is_subscribed = ? WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_write_conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_write_conn));

    sqlite3_bind_int(stmt, 1, subscribed ? 1 : 0);
    sqlite3_bind_int64(stmt, 2, folder_id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_write_conn));

    sqlite3_finalize(stmt);
    return ok;
}

std::vector<Folder> FolderDAL::findByParent(int64_t parent_id, int limit, int offset) const
{
    ReadGuard g(m_pool);
    const char* sql = FOLDER_SELECT
        "WHERE parent_id = ? ORDER BY name ASC LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g.db(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return {};

    sqlite3_bind_int64(stmt, 1, parent_id);
    sqlite3_bind_int (stmt, 2, limit);
    sqlite3_bind_int (stmt, 3, offset);
    return fetchRows(stmt);
}