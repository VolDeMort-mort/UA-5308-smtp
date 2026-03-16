#include "TestFixture.h"
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

    User registerAlice()
    {
        User u;
        u.username = "alice"; u.password_hash = "hash_alice";
        EXPECT_TRUE(repo->registerUser(u));
        return u;
    }
};

// ── registerUser ──────────────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, RegisterUserSucceeds)
{
    User u;
    u.username = "alice"; u.password_hash = "h";
    EXPECT_TRUE(repo->registerUser(u));
    EXPECT_TRUE(u.id.has_value());
}

TEST_F(UserRepositoryTest, RegisterUserDuplicateFails)
{
    registerAlice();

    User dup;
    dup.username = "alice"; dup.password_hash = "other";
    EXPECT_FALSE(repo->registerUser(dup));
    EXPECT_FALSE(repo->getLastError().empty());
}

TEST_F(UserRepositoryTest, RegisterUserErrorMessageDoesNotLeakPassword)
{
    registerAlice();

    User dup;
    dup.username = "alice"; dup.password_hash = "secret_password";
    repo->registerUser(dup);
    EXPECT_EQ(repo->getLastError().find("secret_password"), std::string::npos);
}

// ── authorize ─────────────────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, AuthorizeSucceedsWithCorrectCredentials)
{
    registerAlice();
    EXPECT_TRUE(repo->authorize("alice", "hash_alice"));
}

TEST_F(UserRepositoryTest, AuthorizeFailsWithWrongPassword)
{
    registerAlice();
    EXPECT_FALSE(repo->authorize("alice", "wrong_hash"));
    EXPECT_FALSE(repo->getLastError().empty());
}

TEST_F(UserRepositoryTest, AuthorizeFailsWithUnknownUsername)
{
    EXPECT_FALSE(repo->authorize("nobody", "hash"));
}

TEST_F(UserRepositoryTest, AuthorizeUnknownAndWrongPasswordReturnSameError)
{
    // Timing side-channel fix: both cases must produce the same error string
    registerAlice();

    repo->authorize("alice", "wrong");
    std::string wrongPwErr = repo->getLastError();

    repo->authorize("nobody", "hash");
    std::string noUserErr = repo->getLastError();

    EXPECT_EQ(wrongPwErr, noUserErr);
}

// ── findByID ──────────────────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, FindByIDReturnsUser)
{
    User u = registerAlice();
    auto found = repo->findByID(u.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->username, "alice");
}

TEST_F(UserRepositoryTest, FindByIDUnknownReturnsNullopt)
{
    EXPECT_FALSE(repo->findByID(9999).has_value());
}

// ── findByUsername ────────────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, FindByUsernameReturnsUser)
{
    registerAlice();
    EXPECT_TRUE(repo->findByUsername("alice").has_value());
}

TEST_F(UserRepositoryTest, FindByUsernameUnknownReturnsNullopt)
{
    EXPECT_FALSE(repo->findByUsername("nobody").has_value());
}

// ── findAll ───────────────────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, FindAllReturnsAllUsers)
{
    User a; a.username = "alice"; a.password_hash = "h"; repo->registerUser(a);
    User b; b.username = "bob";   b.password_hash = "h"; repo->registerUser(b);

    auto all = repo->findAll(100);
    EXPECT_EQ(all.size(), 2u);
}

TEST_F(UserRepositoryTest, FindAllPaginationLimitsResults)
{
    for (int i = 0; i < 5; ++i)
    {
        User u;
        u.username      = "user" + std::to_string(i);
        u.password_hash = "h";
        repo->registerUser(u);
    }

    EXPECT_EQ(repo->findAll(2, 0).size(), 2u);
    EXPECT_EQ(repo->findAll(2, 2).size(), 2u);
    EXPECT_EQ(repo->findAll(2, 4).size(), 1u);
    EXPECT_EQ(repo->findAll(2, 6).size(), 0u);
}

// ── changePassword ────────────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, ChangePasswordUpdatesHash)
{
    User u = registerAlice();
    ASSERT_TRUE(repo->changePassword(u.id.value(), "new_hash"));
    EXPECT_TRUE(repo->authorize("alice", "new_hash"));
}

TEST_F(UserRepositoryTest, ChangePasswordOldHashNoLongerWorks)
{
    User u = registerAlice();
    repo->changePassword(u.id.value(), "new_hash");
    EXPECT_FALSE(repo->authorize("alice", "hash_alice"));
}

TEST_F(UserRepositoryTest, ChangePasswordNonExistentUserFails)
{
    EXPECT_FALSE(repo->changePassword(9999, "new_hash"));
}

// ── update ────────────────────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, UpdateChangesUsername)
{
    User u = registerAlice();
    u.username = "alice_v2";
    ASSERT_TRUE(repo->update(u));

    auto found = repo->findByID(u.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->username, "alice_v2");
}

// ── hardDelete ────────────────────────────────────────────────────────────────

TEST_F(UserRepositoryTest, HardDeleteRemovesUser)
{
    User u = registerAlice();
    ASSERT_TRUE(repo->hardDelete(u.id.value()));
    EXPECT_FALSE(repo->findByID(u.id.value()).has_value());
}

TEST_F(UserRepositoryTest, HardDeleteNonExistentFails)
{
    EXPECT_FALSE(repo->hardDelete(9999));
}

TEST_F(UserRepositoryTest, HardDeletedUserCannotAuthorize)
{
    User u = registerAlice();
    repo->hardDelete(u.id.value());
    EXPECT_FALSE(repo->authorize("alice", "hash_alice"));
}