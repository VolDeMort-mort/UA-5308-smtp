#pragma once

#include <vector>
#include <optional>
#include <cstdint>
#include <sqlite3.h>

#include "Entity/User.h"
#include "ConnectionPool.h"

class UserDAL
{
public:
    explicit UserDAL(sqlite3* write_conn, ConnectionPool& pool);

    std::optional<User> findByID(int64_t id) const;
    std::optional<User> findByUsername(const std::string& username) const;
    std::vector<User> findAll(int limit = 50, int offset = 0) const;
    bool existsByUsername(const std::string& username) const;
    int64_t count() const;
    std::optional<std::string> getAvatar(int64_t id) const;

    bool insert(User& user);
    bool update(const User& user);
    bool updatePassword(int64_t id, const std::string& new_password_hash);
    bool updateProfile(int64_t id, const std::optional<std::string>& first_name,
                       const std::optional<std::string>& last_name,
                       const std::optional<std::string>& birthdate);
    bool updateAvatar(int64_t id, const std::optional<std::string>& avatar_b64);
    bool hardDelete(int64_t id);

    const std::string& getLastError() const;

private:
    sqlite3* m_write_conn;
    ConnectionPool& m_pool;
    mutable std::string m_last_error;

    bool setError(const char* sqlite_errmsg);
    std::vector<User> fetchRows(sqlite3_stmt* stmt) const;
    static User rowToUser(sqlite3_stmt* stmt);
};