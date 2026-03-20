#pragma once

#include <optional>
#include <vector>
#include <string>
#include <cstdint>
#include <sodium.h>

#include "../Entity/User.h"
#include "../Entity/Folder.h"
#include "../DAL/UserDAL.h"
#include "../DAL/FolderDAL.h"

class UserRepository
{
public:
    explicit UserRepository(sqlite3* db, UserDAL& user_dal);

    std::optional<User> findByID(int64_t id) const;
    std::optional<User> findByUsername(const std::string& username) const;
    std::vector<User> findAll(int limit = 50, int offset = 0) const;

    bool registerUser(User& user, const std::string& password);
    bool authorize(const std::string& username, const std::string& password);
    bool changePassword(int64_t id, const std::string& new_password);

    bool update(const User& user);
    bool hardDelete(int64_t id);

    const std::string& getLastError() const;

private:
    sqlite3* m_db;

    UserDAL& m_user_dal;
    FolderDAL m_folder_dal;
    mutable std::string m_last_error;

    std::string hashPassword(const std::string& password) const;
    bool setError(const std::string& error) const;
};