#include "TestFixture.h"

struct UserRepositoryTest : public DBFixture
{
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
};

TEST_F(UserRepositoryTest, RegisterUserSetsID)
{
    User u = makeUser();
    EXPECT_TRUE(repo->registerUser(u));
    EXPECT_TRUE(u.id.has_value());
}

TEST_F(UserRepositoryTest, RegisterUserRejectsDuplicate)
{
    User u1 = makeUser("alice"); repo->registerUser(u1);
    User u2 = makeUser("alice");
    EXPECT_FALSE(repo->registerUser(u2));
    EXPECT_FALSE(repo->getLastError().empty());
}

TEST_F(UserRepositoryTest, AuthorizeSucceedsWithCorrectCredentials)
{
    User u = makeUser("alice", "secret"); repo->registerUser(u);
    EXPECT_TRUE(repo->authorize("alice", "secret"));
}

TEST_F(UserRepositoryTest, AuthorizeFailsWithWrongPassword)
{
    User u = makeUser("alice", "secret"); repo->registerUser(u);
    EXPECT_FALSE(repo->authorize("alice", "wrong"));
}

TEST_F(UserRepositoryTest, AuthorizeFailsForUnknownUser)
{
    EXPECT_FALSE(repo->authorize("nobody", "pass"));
}

TEST_F(UserRepositoryTest, ChangePasswordUpdatesHash)
{
    User u = makeUser("alice", "old"); repo->registerUser(u);
    EXPECT_TRUE(repo->changePassword(u.id.value(), "new"));
    EXPECT_TRUE(repo->authorize("alice", "new"));
    EXPECT_FALSE(repo->authorize("alice", "old"));
}

TEST_F(UserRepositoryTest, HardDeleteRemovesUser)
{
    User u = makeUser(); repo->registerUser(u);
    EXPECT_TRUE(repo->hardDelete(u.id.value()));
    EXPECT_FALSE(repo->findByID(u.id.value()).has_value());
}

TEST_F(UserRepositoryTest, HardDeleteFailsForNonexistentUser)
{
    EXPECT_FALSE(repo->hardDelete(9999));
    EXPECT_FALSE(repo->getLastError().empty());
}