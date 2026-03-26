#include "UserRepository.h"

UserRepository::UserRepository(sqlite3* db, UserDAL& user_dal)
    : m_db(db)
    , m_user_dal(user_dal)
    , m_folder_dal(db)
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
    if (!result.has_value())
        m_last_error = m_user_dal.getLastError();
    return result;
}

std::optional<User> UserRepository::findByUsername(const std::string& username) const
{
    auto result = m_user_dal.findByUsername(username);
    if (!result.has_value())
        m_last_error = m_user_dal.getLastError();
    return result;
}

std::vector<User> UserRepository::findAll(int limit, int offset) const
{
    auto result = m_user_dal.findAll(limit, offset);
    m_last_error = m_user_dal.getLastError();
    return result;
}

// FIX: should also create physical folders in data/mailboxes/{user_id}/ if email saving will be distributed by folders
bool UserRepository::registerUser(User& user, const std::string& password)
{
    if (m_user_dal.findByUsername(user.username).has_value())
        return setError("registerUser: username already exists");

    user.password_hash = hashPassword(password);
    if (user.password_hash.empty())
        return setError("registerUser: password hashing failed");

    if (!m_user_dal.insert(user))
        return setError(m_user_dal.getLastError());

    for (const char* name : {"INBOX", "Sent", "Drafts", "Trash", "Spam"})
    {
        Folder f;
        f.user_id       = user.id.value();
        f.name          = name;
        f.next_uid      = 1;
        f.is_subscribed = true;
        if (!m_folder_dal.insert(f))
            return setError(m_folder_dal.getLastError());
    }

    return true;
}

bool UserRepository::authorize(const std::string& username, const std::string& password)
{
    auto user = m_user_dal.findByUsername(username);

    if (!user.has_value())
        return setError("authorize: user not found");

    if (crypto_pwhash_str_verify(
            user->password_hash.c_str(),
            password.c_str(), password.size()) != 0)
        return setError("authorize: invalid credentials");

    return true;
}

bool UserRepository::changePassword(int64_t id, const std::string& new_password)
{
    if (!m_user_dal.findByID(id).has_value())
        return setError("changePassword: user not found");

    if (!m_user_dal.updatePassword(id, hashPassword(new_password)))
        return setError(m_user_dal.getLastError());

    return true;
}

bool UserRepository::update(const User& user)
{
    if (!m_user_dal.update(user))
        return setError(m_user_dal.getLastError());
    return true;
}

bool UserRepository::hardDelete(int64_t id)
{
    if (!m_user_dal.findByID(id).has_value())
        return setError("hardDelete: user not found");

    if (!m_user_dal.hardDelete(id))
        return setError(m_user_dal.getLastError());

    return true;
}

std::string UserRepository::hashPassword(const std::string& password) const
{
    char hash[crypto_pwhash_STRBYTES];

    if (crypto_pwhash_str(
            hash,
            password.c_str(), password.size(),
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0)
    {
        return "";
    }

    return std::string(hash);
}