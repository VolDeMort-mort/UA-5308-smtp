#include "FolderDAL.h"

#define FOLDER_SELECT \
    "SELECT id, user_id, name " \
    "FROM folders "

FolderDAL::FolderDAL(sqlite3* db)
    : m_db(db)
{}

bool FolderDAL::setError(const char* sqlite_errmsg)
{
    m_last_error = sqlite_errmsg ? sqlite_errmsg : "unknown error";
    //Log(Error, <<m_last_error);
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

    const unsigned char* raw = sqlite3_column_text(stmt, 2);
    f.name = raw ? reinterpret_cast<const char*>(raw) : "";

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
    const char* sql = FOLDER_SELECT "WHERE id = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return std::nullopt;

    sqlite3_bind_int64(stmt, 1, id);

    std::optional<Folder> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = rowToFolder(stmt);

    sqlite3_finalize(stmt);
    return result;
}

std::vector<Folder> FolderDAL::findByUser(int64_t user_id) const
{
    const char* sql = FOLDER_SELECT
        "WHERE user_id = ? "
        "ORDER BY name ASC;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return {};

    sqlite3_bind_int64(stmt, 1, user_id);
    return fetchRows(stmt);
}

std::optional<Folder> FolderDAL::findByName(int64_t user_id,
                                             const std::string& name) const
{
    const char* sql = FOLDER_SELECT
        "WHERE user_id = ? AND name = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
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
        "INSERT INTO folders (user_id, name) "
        "VALUES (?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_db));

    sqlite3_bind_int64(stmt, 1, folder.user_id);
    sqlite3_bind_text(stmt, 2, folder.name.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (ok)
        folder.id = sqlite3_last_insert_rowid(m_db);
    else
        setError(sqlite3_errmsg(m_db));

    sqlite3_finalize(stmt);
    return ok;
}

bool FolderDAL::update(const Folder& folder)
{
    if (!folder.id.has_value())
        return setError("update() called on a Folder with no id");

    const char* sql =
        "UPDATE folders "
        "SET user_id = ?, name = ? "
        "WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_db));

    sqlite3_bind_int64(stmt, 1, folder.user_id);
    sqlite3_bind_text(stmt, 2, folder.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, folder.id.value());

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_db));

    sqlite3_finalize(stmt);
    return ok;
}

bool FolderDAL::hardDelete(int64_t id)
{
    const char* sql = "DELETE FROM folders WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_db));

    sqlite3_bind_int64(stmt, 1, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_db));

    sqlite3_finalize(stmt);
    return ok;
}