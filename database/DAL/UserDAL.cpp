#include "UserDAL.h"

#define USER_SELECT \
    "SELECT id, username, password_hash " \
    "From users "

UserDAL::UserDAL(sqlite3* db) : m_db(db) {}

bool UserDAL::setError(const char* sqlite_errmsg)
{
    m_last_error = sqlite_errmsg ? sqlite_errmsg : "unknown error";
    //Log(ERROR, <<m_last_error);
    return false;
}

const std::string& UserDAL::getLastError() const
{
    return m_last_error;
}

User UserDAL::rowToUser(sqlite3_stmt* stmt)
{
    User u;

    if(sqlite3_column_type(stmt, 0) != SQLITE_NULL) u.id = sqlite3_column_int64(stmt, 0);

    auto text = [&](int col) -> std::string
    {
        const unsigned char* raw = sqlite3_column_text(stmt, col);
        return raw ? reinterpret_cast<const char*>(raw) : "";
    };

    u.username = text(1);
    u.password_hash = text(2);

    return u;
}

std::vector<User> UserDAL::fetchRows(sqlite3_stmt* stmt) const
{
    std::vector<User> result;

    while (sqlite3_step(stmt) == SQLITE_ROW) result.push_back(rowToUser(stmt));

    sqlite3_finalize(stmt);

    return result;
}

std::optional<User> UserDAL::findByID(int64_t id) const
{
    const char* sql = USER_SELECT "WHERE id = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int64(stmt, 1, id);

    std::optional<User> result;
    if(sqlite3_step(stmt) == SQLITE_ROW) result = rowToUser(stmt);

    sqlite3_finalize(stmt);

    return result;
}

std::optional<User> UserDAL::findByUsername(const std::string& username) const
{
    const char* sql = USER_SELECT "WHERE username = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr ) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<User> result;
    if(sqlite3_step(stmt) == SQLITE_ROW) result = rowToUser(stmt);

    sqlite3_finalize(stmt);
    return result;
}

std::vector<User> UserDAL::findAll() const
{
    const char* sql = USER_SELECT "ORDER BY username ASC;";

    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return {};

    return fetchRows(stmt);
}

bool UserDAL::insert(User& user)
{
    const char* sql =
        "INSERT INTO users (username, password_hash) "
        "VALUES (?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_db));

    sqlite3_bind_text(stmt, 1, user.username.c_str(),      -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user.password_hash.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (ok)
        user.id = sqlite3_last_insert_rowid(m_db);
    else
        setError(sqlite3_errmsg(m_db));

    sqlite3_finalize(stmt);
    return ok;
}

bool UserDAL::update(const User& user)
{
    if (!user.id.has_value())
        return setError("update() called on a User with no id");

    const char* sql =
        "UPDATE users "
        "SET username = ?, password_hash = ? "
        "WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_db));

    sqlite3_bind_text(stmt, 1, user.username.c_str(),      -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user.password_hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, user.id.value());

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_db));

    sqlite3_finalize(stmt);
    return ok;
}

bool UserDAL::updatePassword(int64_t id, const std::string& new_password_hash)
{
    const char* sql = "UPDATE users SET password_hash = ? WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_db));

    sqlite3_bind_text(stmt, 1, new_password_hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_db));

    sqlite3_finalize(stmt);
    return ok;
}

bool UserDAL::hardDelete(int64_t id)
{
    const char* sql = "DELETE FROM users WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_db));

    sqlite3_bind_int64(stmt, 1, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_db));

    sqlite3_finalize(stmt);
    return ok;
}