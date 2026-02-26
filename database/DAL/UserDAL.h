#pragma once

#include <vector>
#include <optional>
#include <cstdint>

#include "../Entity/User.h"
#include "../sqlite3/sqlite3.h"

class UserDAL
{
public:
    explicit UserDAL(sqlite3* db);

    std::optional<User>  findByID(int64_t id) const;
    std::optional<User> findByUsername(const std::string& username) const;
    std::vector<User> findAll() const;

    bool insert(User& user);
    bool update(const User& user);
    bool updatePassword(int64_t id, const std::string& new_password_hash);
    bool hardDelete(int64_t id);

    const std::string& getLastError() const;

private:
    sqlite3* m_db;
    std::string m_last_error;

    bool setError(const char* sqlite_errmsg);

    std::vector<User> fetchRows(sqlite3_stmt* stmt) const;
    static User rowToUser(sqlite3_stmt* stmt); 
};