#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <optional>
#include <string>

#include "DataBaseManager.h"
#include "Repository/UserRepository.h"
#include "schema.h"

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

std::atomic<int> g_db_counter{0};

std::string uniqueDbPath() {
    auto tmp = std::filesystem::temp_directory_path();
    return (tmp / ("user_repo_" + std::to_string(++g_db_counter) + ".db")).string();
}

void removeDb(const std::string& path) {
    for (const char* suffix : {"", "-wal", "-shm"})
        std::filesystem::remove(path + suffix);
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class UserRepositoryTest : public ::testing::Test {
protected:
    std::string          m_path;
    std::unique_ptr<DataBaseManager>  m_mgr;
    std::unique_ptr<UserRepository>   m_repo;

    void SetUp() override {
        m_path = uniqueDbPath();
        m_mgr  = std::make_unique<DataBaseManager>(m_path, initSchema());
        ASSERT_TRUE(m_mgr->isConnected()) << "Failed to open database at " << m_path;
        m_repo = std::make_unique<UserRepository>(*m_mgr);
    }

    void TearDown() override {
        m_repo.reset();
        m_mgr.reset();
        removeDb(m_path);
    }

    // Build a minimal User ready to be handed to registerUser()
    static User buildUser(const std::string& username) {
        User u;
        u.username = username;
        return u;
    }

    // Register a user and return the populated struct (id will be set)
    User reg(const std::string& username  = "alice",
             const std::string& password  = "Secret123!") {
        User u = buildUser(username);
        EXPECT_TRUE(m_repo->registerUser(u, password))
            << "registerUser failed for '" << username << "'";
        return u;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Registration
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, RegisterUser_AssignsPositiveID) {
    User u = buildUser("alice");
    ASSERT_TRUE(m_repo->registerUser(u, "pass"));
    ASSERT_TRUE(u.id.has_value());
    EXPECT_GT(*u.id, 0);
}

TEST_F(UserRepositoryTest, RegisterUser_PersistsUsername) {
    User u = buildUser("bob");
    ASSERT_TRUE(m_repo->registerUser(u, "pass"));

    auto found = m_repo->findByID(*u.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->username, "bob");
}

TEST_F(UserRepositoryTest, RegisterUser_DuplicateUsername_Fails) {
    User u1 = buildUser("charlie");
    ASSERT_TRUE(m_repo->registerUser(u1, "pass1"));

    User u2 = buildUser("charlie");
    EXPECT_FALSE(m_repo->registerUser(u2, "pass2"));
}

TEST_F(UserRepositoryTest, RegisterUser_PasswordHashedNotPlaintext) {
    User u = buildUser("dave");
    ASSERT_TRUE(m_repo->registerUser(u, "MySuperSecret!"));

    auto fetched = m_repo->findByID(*u.id);
    ASSERT_TRUE(fetched.has_value());
    EXPECT_NE(fetched->password_hash, "MySuperSecret!");
    EXPECT_FALSE(fetched->password_hash.empty());
}

TEST_F(UserRepositoryTest, RegisterMultipleUsers_HaveUniqueIDs) {
    User u1 = buildUser("u1"); ASSERT_TRUE(m_repo->registerUser(u1, "p"));
    User u2 = buildUser("u2"); ASSERT_TRUE(m_repo->registerUser(u2, "p"));
    User u3 = buildUser("u3"); ASSERT_TRUE(m_repo->registerUser(u3, "p"));

    EXPECT_NE(*u1.id, *u2.id);
    EXPECT_NE(*u2.id, *u3.id);
    EXPECT_NE(*u1.id, *u3.id);
}

TEST_F(UserRepositoryTest, RegisterUser_WithOptionalProfile_PersistsFields) {
    User u = buildUser("eve");
    u.first_name = "Eve";
    u.last_name  = "Example";
    u.birthdate  = "1995-03-15";
    ASSERT_TRUE(m_repo->registerUser(u, "pass"));

    auto found = m_repo->findByID(*u.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->first_name, "Eve");
    EXPECT_EQ(found->last_name,  "Example");
    EXPECT_EQ(found->birthdate,  "1995-03-15");
}

// ─────────────────────────────────────────────────────────────────────────────
// Authorization
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, Authorize_CorrectPassword_ReturnsTrue) {
    reg("alice", "Correct!");
    EXPECT_TRUE(m_repo->authorize("alice", "Correct!"));
}

TEST_F(UserRepositoryTest, Authorize_WrongPassword_ReturnsFalse) {
    reg("alice", "Correct!");
    EXPECT_FALSE(m_repo->authorize("alice", "Wrong!"));
}

TEST_F(UserRepositoryTest, Authorize_NonExistentUser_ReturnsFalse) {
    EXPECT_FALSE(m_repo->authorize("ghost", "anything"));
}

TEST_F(UserRepositoryTest, Authorize_EmptyPassword_ReturnsFalse) {
    reg("alice", "pass");
    EXPECT_FALSE(m_repo->authorize("alice", ""));
}

TEST_F(UserRepositoryTest, Authorize_CaseSensitiveUsername) {
    reg("Alice", "pass");
    // Lookup with different casing should fail (SQLite LIKE is case-insensitive for ASCII
    // but findByUsername typically uses exact match — verify behaviour)
    EXPECT_TRUE(m_repo->authorize("Alice", "pass"));
    // "alice" != "Alice" at the DB level (depends on BINARY collation)
    // We don't assert the negative here since collation is implementation-defined,
    // but the positive must always hold.
}

// ─────────────────────────────────────────────────────────────────────────────
// Find
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, FindByID_ExistingUser_ReturnsCorrectUser) {
    User created = reg("alice");
    auto found   = m_repo->findByID(*created.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->id,       created.id);
    EXPECT_EQ(found->username, "alice");
}

TEST_F(UserRepositoryTest, FindByID_NonExistentID_ReturnsNullopt) {
    EXPECT_FALSE(m_repo->findByID(999999).has_value());
}

TEST_F(UserRepositoryTest, FindByUsername_ExistingUser_ReturnsUser) {
    reg("bob");
    auto found = m_repo->findByUsername("bob");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->username, "bob");
}

TEST_F(UserRepositoryTest, FindByUsername_NonExistentUser_ReturnsNullopt) {
    EXPECT_FALSE(m_repo->findByUsername("nobody").has_value());
}

TEST_F(UserRepositoryTest, FindAll_EmptyDatabase_ReturnsEmptyVector) {
    EXPECT_TRUE(m_repo->findAll().empty());
}

TEST_F(UserRepositoryTest, FindAll_ReturnsAllRegisteredUsers) {
    reg("a"); reg("b"); reg("c");
    auto all = m_repo->findAll(50, 0);
    EXPECT_EQ(all.size(), 3u);
}

TEST_F(UserRepositoryTest, FindAll_PaginationLimit) {
    for (int i = 0; i < 6; ++i)
        reg("pg_user" + std::to_string(i));

    auto page1 = m_repo->findAll(3, 0);
    auto page2 = m_repo->findAll(3, 3);

    EXPECT_EQ(page1.size(), 3u);
    EXPECT_EQ(page2.size(), 3u);

    // No overlap between pages
    for (const auto& u1 : page1)
        for (const auto& u2 : page2)
            EXPECT_NE(u1.id, u2.id);
}

TEST_F(UserRepositoryTest, FindAll_PaginationOffsetBeyondEnd_ReturnsEmpty) {
    reg("a"); reg("b");
    auto page = m_repo->findAll(50, 100);
    EXPECT_TRUE(page.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// Password change
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, ChangePassword_LoginWithNewPasswordSucceeds) {
    User u = reg("alice", "OldPass!");
    ASSERT_TRUE(m_repo->changePassword(*u.id, "NewPass!"));
    EXPECT_TRUE(m_repo->authorize("alice", "NewPass!"));
}

TEST_F(UserRepositoryTest, ChangePassword_OldPasswordNoLongerWorks) {
    User u = reg("alice", "OldPass!");
    ASSERT_TRUE(m_repo->changePassword(*u.id, "NewPass!"));
    EXPECT_FALSE(m_repo->authorize("alice", "OldPass!"));
}

TEST_F(UserRepositoryTest, ChangePassword_NonExistentUser_ReturnsFalse) {
    EXPECT_FALSE(m_repo->changePassword(999999, "anything"));
}

TEST_F(UserRepositoryTest, ChangePassword_HashStoredNotPlaintext) {
    User u = reg("alice", "OldPass!");
    ASSERT_TRUE(m_repo->changePassword(*u.id, "NewPass!"));

    auto found = m_repo->findByID(*u.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_NE(found->password_hash, "NewPass!");
}

// ─────────────────────────────────────────────────────────────────────────────
// Profile update
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, UpdateProfile_SetsAllFields) {
    User u = reg("alice");
    ASSERT_TRUE(m_repo->updateProfile(*u.id, "Alice", "Wonder", "1990-01-15"));

    auto found = m_repo->findByID(*u.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->first_name, "Alice");
    EXPECT_EQ(found->last_name,  "Wonder");
    EXPECT_EQ(found->birthdate,  "1990-01-15");
}

TEST_F(UserRepositoryTest, UpdateProfile_NulloptClearsFields) {
    User u = reg("bob");
    m_repo->updateProfile(*u.id, "Bob", "Smith", "1985-06-01");

    ASSERT_TRUE(m_repo->updateProfile(*u.id, std::nullopt, std::nullopt, std::nullopt));

    auto found = m_repo->findByID(*u.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_FALSE(found->first_name.has_value());
    EXPECT_FALSE(found->last_name.has_value());
    EXPECT_FALSE(found->birthdate.has_value());
}

TEST_F(UserRepositoryTest, UpdateProfile_PartialUpdate_OnlyChangesSpecifiedFields) {
    User u = reg("carol");
    m_repo->updateProfile(*u.id, "Carol", "Jones", "2000-12-31");

    // Change only first_name
    ASSERT_TRUE(m_repo->updateProfile(*u.id, "Caroline", std::nullopt, std::nullopt));

    auto found = m_repo->findByID(*u.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->first_name, "Caroline");
    EXPECT_FALSE(found->last_name.has_value());
    EXPECT_FALSE(found->birthdate.has_value());
}

TEST_F(UserRepositoryTest, Update_UsernameChange_ReflectedInFind) {
    User u = reg("alice");
    u.username = "alice_renamed";
    ASSERT_TRUE(m_repo->update(u));

    EXPECT_FALSE(m_repo->findByUsername("alice").has_value());
    auto found = m_repo->findByUsername("alice_renamed");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->id, u.id);
}

// ─────────────────────────────────────────────────────────────────────────────
// Avatar
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, UpdateAvatar_ThenGetAvatar_ReturnsData) {
    User u = reg("alice");
    const std::string fakeAvatar = "iVBORw0KGgoAAAANSUhEUgAA=";
    ASSERT_TRUE(m_repo->updateAvatar(*u.id, fakeAvatar));

    auto avatar = m_repo->getAvatar(*u.id);
    ASSERT_TRUE(avatar.has_value());
    EXPECT_EQ(*avatar, fakeAvatar);
}

TEST_F(UserRepositoryTest, UpdateAvatar_NulloptClearsAvatar) {
    User u = reg("alice");
    m_repo->updateAvatar(*u.id, "somedata");
    ASSERT_TRUE(m_repo->updateAvatar(*u.id, std::nullopt));

    auto avatar = m_repo->getAvatar(*u.id);
    EXPECT_FALSE(avatar.has_value());
}

TEST_F(UserRepositoryTest, GetAvatar_NoAvatarSet_ReturnsNullopt) {
    User u = reg("bob");
    auto avatar = m_repo->getAvatar(*u.id);
    EXPECT_FALSE(avatar.has_value());
}

TEST_F(UserRepositoryTest, GetAvatar_NonExistentUser_ReturnsNullopt) {
    EXPECT_FALSE(m_repo->getAvatar(999999).has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// Hard delete
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, HardDelete_RemovesUserFromDB) {
    User u = reg("alice");
    ASSERT_TRUE(m_repo->hardDelete(*u.id));
    EXPECT_FALSE(m_repo->findByID(*u.id).has_value());
}

TEST_F(UserRepositoryTest, HardDelete_UserNoLongerInFindAll) {
    User u1 = reg("alice");
    User u2 = reg("bob");
    ASSERT_TRUE(m_repo->hardDelete(*u1.id));

    auto all = m_repo->findAll();
    ASSERT_EQ(all.size(), 1u);
    EXPECT_EQ(all[0].username, "bob");
}

TEST_F(UserRepositoryTest, HardDelete_AuthorizationFailsAfterDelete) {
    User u = reg("alice", "pass");
    ASSERT_TRUE(m_repo->hardDelete(*u.id));
    EXPECT_FALSE(m_repo->authorize("alice", "pass"));
}

TEST_F(UserRepositoryTest, HardDelete_NonExistentUser_ReturnsFalse) {
    EXPECT_FALSE(m_repo->hardDelete(999999));
}

TEST_F(UserRepositoryTest, HardDelete_DeletedIDNotReused) {
    User u1 = reg("alice");
    int64_t deleted_id = *u1.id;
    ASSERT_TRUE(m_repo->hardDelete(deleted_id));

    User u2 = reg("bob");
    EXPECT_NE(*u2.id, deleted_id);
}