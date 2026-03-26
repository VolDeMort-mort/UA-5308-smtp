#include "UserDAL.h"

#define USER_SELECT \
    "SELECT id, username, password_hash, first_name, last_name, birthdate, avatar_b64 " \
    "FROM users "

#define USER_SELECT_NO_AVATAR \
    "SELECT id, username, password_hash, first_name, last_name, birthdate, NULL " \
    "FROM users "

UserDAL::UserDAL(sqlite3* write_conn, ConnectionPool& pool)
    : m_write_conn(write_conn)
    , m_pool(pool)
{}

bool UserDAL::setError(const char* sqlite_errmsg)
{
    m_last_error = sqlite_errmsg ? sqlite_errmsg : "unknown error";
    return false;
}

const std::string& UserDAL::getLastError() const
{
    return m_last_error;
}

User UserDAL::rowToUser(sqlite3_stmt* stmt)
{
    User u;

    if (sqlite3_column_type(stmt, 0) != SQLITE_NULL)
        u.id = sqlite3_column_int64(stmt, 0);

    auto text = [&](int col) -> std::string
    {
        const unsigned char* raw = sqlite3_column_text(stmt, col);
        return raw ? reinterpret_cast<const char*>(raw) : "";
    };

    auto optText = [&](int col) -> std::optional<std::string>
    {
        if (sqlite3_column_type(stmt, col) == SQLITE_NULL) return std::nullopt;
        const unsigned char* raw = sqlite3_column_text(stmt, col);
        return raw ? std::optional<std::string>(reinterpret_cast<const char*>(raw)) : std::nullopt;
    };

    u.username      = text(1);
    u.password_hash = text(2);
    u.first_name    = optText(3);
    u.last_name     = optText(4);
    u.birthdate     = optText(5);
    u.avatar_b64    = optText(6);

    return u;
}

std::vector<User> UserDAL::fetchRows(sqlite3_stmt* stmt) const
{
    std::vector<User> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(rowToUser(stmt));
    sqlite3_finalize(stmt);
    return result;
}

std::optional<User> UserDAL::findByID(int64_t id) const
{
    ReadGuard g(m_pool);
    const char* sql = USER_SELECT "WHERE id = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g.db(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return std::nullopt;

    sqlite3_bind_int64(stmt, 1, id);

    std::optional<User> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = rowToUser(stmt);

    sqlite3_finalize(stmt);
    return result;
}

std::optional<User> UserDAL::findByUsername(const std::string& username) const
{
    ReadGuard g(m_pool);
    const char* sql = USER_SELECT_NO_AVATAR "WHERE username = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g.db(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return std::nullopt;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<User> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = rowToUser(stmt);

    sqlite3_finalize(stmt);
    return result;
}

std::vector<User> UserDAL::findAll(int limit, int offset) const
{
    ReadGuard g(m_pool);
    const char* sql = USER_SELECT_NO_AVATAR "ORDER BY username ASC LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g.db(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return {};

    sqlite3_bind_int(stmt, 1, limit);
    sqlite3_bind_int(stmt, 2, offset);
    return fetchRows(stmt);
}

bool UserDAL::existsByUsername(const std::string& username) const
{
    ReadGuard g(m_pool);
    const char* sql = "SELECT 1 FROM users WHERE username = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g.db(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return exists;
}

int64_t UserDAL::count() const
{
    ReadGuard g(m_pool);
    const char* sql = "SELECT COUNT(*) FROM users;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g.db(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return -1;

    int64_t result = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = sqlite3_column_int64(stmt, 0);

    sqlite3_finalize(stmt);
    return result;
}

std::optional<std::string> UserDAL::getAvatar(int64_t id) const
{
    ReadGuard g(m_pool);
    const char* sql = "SELECT avatar_b64 FROM users WHERE id = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g.db(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return std::nullopt;

    sqlite3_bind_int64(stmt, 1, id);

    std::optional<std::string> result;
    if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_type(stmt, 0) != SQLITE_NULL)
    {
        const unsigned char* raw = sqlite3_column_text(stmt, 0);
        if (raw) result = reinterpret_cast<const char*>(raw);
    }

    sqlite3_finalize(stmt);
    return result;
}

bool UserDAL::insert(User& user)
{
    const char* sql =
        "INSERT INTO users (username, password_hash, first_name, last_name, birthdate, avatar_b64) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_write_conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_write_conn));

    auto bindOpt = [&](int pos, const std::optional<std::string>& val)
    {
        if (val.has_value()) sqlite3_bind_text(stmt, pos, val->c_str(), -1, SQLITE_TRANSIENT);
        else sqlite3_bind_null(stmt, pos);
    };

    sqlite3_bind_text(stmt, 1, user.username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user.password_hash.c_str(), -1, SQLITE_TRANSIENT);
    bindOpt(3, user.first_name);
    bindOpt(4, user.last_name);
    bindOpt(5, user.birthdate);
    bindOpt(6, user.avatar_b64);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (ok)
        user.id = sqlite3_last_insert_rowid(m_write_conn);
    else
        setError(sqlite3_errmsg(m_write_conn));

    sqlite3_finalize(stmt);
    return ok;
}

bool UserDAL::update(const User& user)
{
    if (!user.id.has_value())
        return setError("update() called on a User with no id");

    const char* sql =
        "UPDATE users SET username = ?, password_hash = ?, "
        "first_name = ?, last_name = ?, birthdate = ?, avatar_b64 = ? "
        "WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_write_conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_write_conn));

    auto bindOpt = [&](int pos, const std::optional<std::string>& val)
    {
        if (val.has_value()) sqlite3_bind_text(stmt, pos, val->c_str(), -1, SQLITE_TRANSIENT);
        else sqlite3_bind_null(stmt, pos);
    };

    sqlite3_bind_text(stmt, 1, user.username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user.password_hash.c_str(), -1, SQLITE_TRANSIENT);
    bindOpt(3, user.first_name);
    bindOpt(4, user.last_name);
    bindOpt(5, user.birthdate);
    bindOpt(6, user.avatar_b64);
    sqlite3_bind_int64(stmt, 7, user.id.value());

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_write_conn));

    sqlite3_finalize(stmt);
    return ok;
}

bool UserDAL::updatePassword(int64_t id, const std::string& new_password_hash)
{
    const char* sql = "UPDATE users SET password_hash = ? WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_write_conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_write_conn));

    sqlite3_bind_text(stmt, 1, new_password_hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_write_conn));

    sqlite3_finalize(stmt);
    return ok;
}

bool UserDAL::updateProfile(int64_t id, const std::optional<std::string>& first_name,
                            const std::optional<std::string>& last_name,
                            const std::optional<std::string>& birthdate)
{
    const char* sql =
        "UPDATE users SET first_name = ?, last_name = ?, birthdate = ? "
        "WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_write_conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_write_conn));

    auto bindOpt = [&](int pos, const std::optional<std::string>& val)
    {
        if (val.has_value()) sqlite3_bind_text(stmt, pos, val->c_str(), -1, SQLITE_TRANSIENT);
        else sqlite3_bind_null(stmt, pos);
    };

    bindOpt(1, first_name);
    bindOpt(2, last_name);
    bindOpt(3, birthdate);
    sqlite3_bind_int64(stmt, 4, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_write_conn));

    sqlite3_finalize(stmt);
    return ok;
}

bool UserDAL::updateAvatar(int64_t id, const std::optional<std::string>& avatar_b64)
{
    const char* sql = "UPDATE users SET avatar_b64 = ? WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_write_conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_write_conn));

    if (avatar_b64.has_value())
        sqlite3_bind_text(stmt, 1, avatar_b64->c_str(), -1, SQLITE_TRANSIENT);
    else
        sqlite3_bind_null(stmt, 1);

    sqlite3_bind_int64(stmt, 2, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_write_conn));

    sqlite3_finalize(stmt);
    return ok;
}

bool UserDAL::hardDelete(int64_t id)
{
    const char* sql = "DELETE FROM users WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_write_conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return setError(sqlite3_errmsg(m_write_conn));

    sqlite3_bind_int64(stmt, 1, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) setError(sqlite3_errmsg(m_write_conn));

    sqlite3_finalize(stmt);
    return ok;
}