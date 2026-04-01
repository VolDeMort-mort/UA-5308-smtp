#include <optional>
#include <string>

#include "DataBaseManager.h"
#include "Repository/UserRepository.h"
#include "schema.h"

namespace
{
std::string normalizeRoot(std::string root)
{
    while (!root.empty() && (root.back() == '/' || root.back() == '\\'))
    {
        root.pop_back();
    }

    if (!root.empty())
    {
        root.push_back('/');
    }

    return root;
}

bool ensureUser(UserRepository& repo,
                const std::string& username,
                const std::string& password,
                const std::optional<std::string>& firstName,
                const std::optional<std::string>& lastName,
                const std::optional<std::string>& birthdate)
{
    auto existing = repo.findByUsername(username);
    if (!existing.has_value())
    {
        User user;
        user.username = username;
        user.first_name = firstName;
        user.last_name = lastName;
        user.birthdate = birthdate;
        user.avatar_b64 = std::nullopt;

        if (!repo.registerUser(user, password))
        {
            return false;
        }
        return true;
    }

    if (!repo.changePassword(existing->id.value(), password))
    {
        return false;
    }

    if (!repo.updateProfile(existing->id.value(), firstName, lastName, birthdate))
    {
        return false;
    }

    if (!repo.updateAvatar(existing->id.value(), std::nullopt))
    {
        return false;
    }
    return true;
}
} // namespace

int main(int argc, char** argv)
{
    std::string root = "./";
    if (argc >= 2 && argv[1] != nullptr)
    {
        root = argv[1];
    }
    root = normalizeRoot(root);

    const std::string dbPath = root + "data/mail.db";
    DataBaseManager db(dbPath, initSchema());
    if (!db.isConnected())
    {
        return 1;
    }

    UserRepository repo(db);

    bool ok = true;
    ok = ensureUser(repo, "alice", "pass", "Alice", "User", "1990-01-01") && ok;
    ok = ensureUser(repo, "bob", "pass", "Bob", "User", "1991-01-01") && ok;

    if (!ok)
    {
        return 1;
    }
    return 0;
}
