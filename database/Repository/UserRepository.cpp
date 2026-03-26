#include "UserRepository.h"

UserRepository::UserRepository(DataBaseManager& db)
    : m_db(db)
    , m_user_dal(db.getDB(), db.pool())
    , m_folder_dal(db.getDB(), db.pool())
{
}

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

bool UserRepository::registerUser(User& user, const std::string& password)
{
    if (m_user_dal.findByUsername(user.username).has_value())
        return setError("registerUser: username already exists");

    user.password_hash = hashPassword(password);
    if (user.password_hash.empty())
        return setError("registerUser: password hashing failed");

    auto lock = m_db.writeLock();

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

    if (crypto_pwhash_str_verify(user->password_hash.c_str(), password.c_str(), password.size()) != 0)
        return setError("authorize: invalid credentials");

    return true;
}

bool UserRepository::changePassword(int64_t id, const std::string& new_password)
{
    if (!m_user_dal.findByID(id).has_value())
        return setError("changePassword: user not found");

    auto lock = m_db.writeLock();

    if (!m_user_dal.updatePassword(id, hashPassword(new_password)))
        return setError(m_user_dal.getLastError());

    return true;
}

bool UserRepository::update(const User& user)
{
    auto lock = m_db.writeLock();
    if (!m_user_dal.update(user))
        return setError(m_user_dal.getLastError());
    return true;
}

bool UserRepository::hardDelete(int64_t id)
{
    if (!m_user_dal.findByID(id).has_value())
        return setError("hardDelete: user not found");

    auto lock = m_db.writeLock();

    if (!m_user_dal.hardDelete(id))
        return setError(m_user_dal.getLastError());

    return true;
}

bool UserRepository::updateProfile(int64_t id, const std::optional<std::string>& first_name,
                                   const std::optional<std::string>& last_name,
                                   const std::optional<std::string>& birthdate)
{
    auto lock = m_db.writeLock();
    if (!m_user_dal.updateProfile(id, first_name, last_name, birthdate))
        return setError(m_user_dal.getLastError());
    return true;
}

bool UserRepository::updateAvatar(int64_t id, const std::optional<std::string>& avatar_b64)
{
    auto lock = m_db.writeLock();
    if (!m_user_dal.updateAvatar(id, avatar_b64))
        return setError(m_user_dal.getLastError());
    return true;
}

std::optional<std::string> UserRepository::getAvatar(int64_t id) const
{
    return m_user_dal.getAvatar(id);
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