#include "UserRepository.h"

UserRepository::UserRepository(UserDAL& user_dal)
    : m_user_dal(user_dal)
{}

bool UserRepository::setError(const std::string& error) const
{
    m_last_error = error;
    return false;
}

const std::string& UserRepository::getLastError() const
{
    return m_last_error;
}

std::optional<User> UserRepository::findByID(int64_t id) const
{
    auto result = m_user_dal.findByID(id);
    if (!result.has_value()){
        m_last_error = m_user_dal.getLastError();
    }

    return result;
}

std::optional<User> UserRepository::findByUsername(const std::string& username) const
{
    auto result = m_user_dal.findByUsername(username);
    if (!result.has_value()){
        m_last_error = m_user_dal.getLastError();
    }

    return result;
}

std::vector<User> UserRepository::findAll() const
{
    auto result = m_user_dal.findAll();
    m_last_error = m_user_dal.getLastError();
    return result;
}


bool UserRepository::registerUser(User& user)
{
    auto existing = m_user_dal.findByUsername(user.username);
    if (existing.has_value()){
        return setError("registerUser: Username already exists");
    }

    if (!m_user_dal.insert(user)){
        return setError(m_user_dal.getLastError());
    }

    return true;
}

bool UserRepository::authorize(const std::string& username, const std::string& password_hash)
{
    auto user = m_user_dal.findByUsername(username);

    if (!user.has_value()){
        return setError("authorize: User not found");
    }

    if (user->password_hash != password_hash){
        return setError("authorize: Invalid credentials");
    }

    return true;
}

bool UserRepository::changePassword(int64_t id, const std::string& new_password_hash)
{
    if (!m_user_dal.findByID(id).has_value()){
        return setError("changePassword: user not found");
    }

    return m_user_dal.updatePassword(id, new_password_hash);
}

bool UserRepository::update(const User& user)
{
    if (!m_user_dal.update(user))
    {
        return setError(m_user_dal.getLastError());
    }

    return true;
}

bool UserRepository::hardDelete(int64_t id)
{
    if (!m_user_dal.findByID(id).has_value())
    {
        return setError("hardDelete: user not found");
    }

    return m_user_dal.hardDelete(id);
}