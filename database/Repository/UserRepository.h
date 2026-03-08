#pragma once

#include <optional>
#include <vector>
#include <string>
#include <cstdint>

#include "../Entity/User.h"
#include "../DAL/UserDAL.h"

class UserRepository
{
public:
    explicit UserRepository(UserDAL& user_dal);

    std::optional<User> findByID(int64_t id) const;
    std::optional<User> findByUsername(const std::string& username) const;
    std::vector<User> findAll() const;

    bool registerUser(User& user);
    bool authorize(const std::string& username, const std::string& password_hash);
    bool changePassword(int64_t id, const std::string& new_password_hash);

    bool update(const User& user);
    bool hardDelete(int64_t id);

    const std::string& getLastError() const;

private:
    UserDAL& m_user_dal;
    mutable std::string m_last_error;

    bool setError(const std::string& error) const;
};