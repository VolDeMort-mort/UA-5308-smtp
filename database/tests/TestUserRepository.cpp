#include "TestHelper.h"
#include "../DAL/UserDAL.h"
#include "../Repository/UserRepository.h"

class UserRepositoryTest : public DBFixture
{
protected:
    UserDAL*        dal  = nullptr;
    UserRepository* repo = nullptr;

    void SetUp() override
    {
        DBFixture::SetUp();
        dal  = new UserDAL(db);
        repo = new UserRepository(*dal);
    }

    void TearDown() override
    {
        delete repo;
        delete dal;
        DBFixture::TearDown();
    }

    User makeUser(const std::string& username)
    {
        User u; u.username = username;
        return u;
    }
};

// ── registerUser ─────────────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, RegisterUserSuccess)
{
    User u = makeUser("alice");
    ASSERT_TRUE(repo->registerUser(u, "secret"));
    ASSERT_TRUE(u.id.has_value());
}

TEST_F(UserRepositoryTest, RegisterUserHashesPassword)
{
    User u = makeUser("bob");
    ASSERT_TRUE(repo->registerUser(u, "plaintext"));

    auto stored = dal->findByID(u.id.value());
    ASSERT_TRUE(stored.has_value());
    // Must NOT store plaintext
    EXPECT_NE(stored->password_hash, "plaintext");
    EXPECT_FALSE(stored->password_hash.empty());
}

TEST_F(UserRepositoryTest, RegisterDuplicateUsernameFails)
{
    User u1 = makeUser("dup");
    ASSERT_TRUE(repo->registerUser(u1, "pass1"));

    User u2 = makeUser("dup");
    EXPECT_FALSE(repo->registerUser(u2, "pass2"));
    EXPECT_FALSE(repo->getLastError().empty());
}

// ── authorize ────────────────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, AuthorizeCorrectCredentials)
{
    User u = makeUser("carol");
    ASSERT_TRUE(repo->registerUser(u, "mypassword"));
    EXPECT_TRUE(repo->authorize("carol", "mypassword"));
}

TEST_F(UserRepositoryTest, AuthorizeWrongPasswordFails)
{
    User u = makeUser("dave");
    ASSERT_TRUE(repo->registerUser(u, "correct"));
    EXPECT_FALSE(repo->authorize("dave", "wrong"));
    EXPECT_FALSE(repo->getLastError().empty());
}

TEST_F(UserRepositoryTest, AuthorizeNonExistentUserFails)
{
    EXPECT_FALSE(repo->authorize("nobody", "pass"));
    EXPECT_FALSE(repo->getLastError().empty());
}

TEST_F(UserRepositoryTest, AuthorizeDoesNotLeakWhichFieldFailed)
{
    // Both "user not found" and "wrong password" must set an error — callers
    // should not be able to distinguish them from the bool return alone.
    User u = makeUser("eve");
    ASSERT_TRUE(repo->registerUser(u, "pass"));

    bool wrong_user = repo->authorize("nobody", "pass");
    bool wrong_pass = repo->authorize("eve", "wrong");

    EXPECT_FALSE(wrong_user);
    EXPECT_FALSE(wrong_pass);
}

// ── changePassword ───────────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, ChangePasswordAllowsLoginWithNewPassword)
{
    User u = makeUser("frank");
    ASSERT_TRUE(repo->registerUser(u, "old"));

    ASSERT_TRUE(repo->changePassword(u.id.value(), "new"));

    EXPECT_TRUE (repo->authorize("frank", "new"));
    EXPECT_FALSE(repo->authorize("frank", "old"));
}

TEST_F(UserRepositoryTest, ChangePasswordNonExistentUserFails)
{
    EXPECT_FALSE(repo->changePassword(9999, "newpass"));
    EXPECT_FALSE(repo->getLastError().empty());
}

// ── findByID ─────────────────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, FindByIDSuccess)
{
    User u = makeUser("grace");
    ASSERT_TRUE(repo->registerUser(u, "p"));

    auto found = repo->findByID(u.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->username, "grace");
}

TEST_F(UserRepositoryTest, FindByIDMissingReturnsNullopt)
{
    EXPECT_FALSE(repo->findByID(9999).has_value());
}

// ── findByUsername ───────────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, FindByUsernameSuccess)
{
    User u = makeUser("heidi");
    ASSERT_TRUE(repo->registerUser(u, "p"));
    EXPECT_TRUE(repo->findByUsername("heidi").has_value());
}

// ── update ───────────────────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, UpdateUsernameSuccess)
{
    User u = makeUser("ivan");
    ASSERT_TRUE(repo->registerUser(u, "p"));

    u.username = "ivan2";
    ASSERT_TRUE(repo->update(u));
    EXPECT_TRUE(repo->findByUsername("ivan2").has_value());
    EXPECT_FALSE(repo->findByUsername("ivan").has_value());
}

// ── hardDelete ───────────────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, HardDeleteRemovesUser)
{
    User u = makeUser("judy");
    ASSERT_TRUE(repo->registerUser(u, "p"));
    ASSERT_TRUE(repo->hardDelete(u.id.value()));
    EXPECT_FALSE(repo->findByID(u.id.value()).has_value());
}

TEST_F(UserRepositoryTest, HardDeleteNonExistentFails)
{
    EXPECT_FALSE(repo->hardDelete(9999));
    EXPECT_FALSE(repo->getLastError().empty());
}

// ── findAll pagination ───────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, FindAllPagination)
{
    for (int i = 0; i < 8; ++i)
    {
        User u = makeUser("user" + std::to_string(i));
        ASSERT_TRUE(repo->registerUser(u, "p"));
    }
    EXPECT_EQ(repo->findAll(3, 0).size(), 3u);
    EXPECT_EQ(repo->findAll(3, 3).size(), 3u);
    EXPECT_EQ(repo->findAll(3, 6).size(), 2u);
}