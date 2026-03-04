#include <gtest/gtest.h>

#include "../Repository/UserRepository.h"
#include "../DAL/UserDAL.h"
#include "../DataBaseManager.h"

class UserRepositoryTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_db = new DataBaseManager(":memory:", "scheme/001_init_scheme.sql");
        m_dal = new UserDAL(m_db->getDB());
        m_repo = new UserRepository(*m_dal);
    }

    void TearDown() override
    {
        delete m_repo;
        delete m_dal;
        delete m_db;
    }

    User make(const std::string& username = "alice", const std::string& hash = "hash")
    {
        User u;
        u.username = username;
        u.password_hash = hash;
        return u;
    }

    DataBaseManager* m_db   = nullptr;
    UserDAL* m_dal  = nullptr;
    UserRepository* m_repo = nullptr;
};

TEST_F(UserRepositoryTest, RegisterUser_SetsID)
{
    auto u = make();
    EXPECT_TRUE(m_repo->registerUser(u));
    EXPECT_TRUE(u.id.has_value());
}

TEST_F(UserRepositoryTest, RegisterUser_PersistsFields)
{
    auto u = make("charlie", "secret");
    m_repo->registerUser(u);
    auto f = m_repo->findByID(u.id.value());
    ASSERT_TRUE(f.has_value());
    EXPECT_EQ(f->username,      "charlie");
    EXPECT_EQ(f->password_hash, "secret");
}

TEST_F(UserRepositoryTest, RegisterUser_RejectsDuplicateUsername)
{
    m_repo->registerUser(make("alice"));
    EXPECT_FALSE(m_repo->registerUser(make("alice")));
}

TEST_F(UserRepositoryTest, Authorize_SucceedsWithCorrectCredentials)
{
    m_repo->registerUser(make("alice", "correct"));
    EXPECT_TRUE(m_repo->authorize("alice", "correct"));
}

TEST_F(UserRepositoryTest, Authorize_FailsWithWrongPassword)
{
    m_repo->registerUser(make("alice", "correct"));
    EXPECT_FALSE(m_repo->authorize("alice", "wrong"));
}

TEST_F(UserRepositoryTest, Authorize_FailsForUnknownUser)
{
    EXPECT_FALSE(m_repo->authorize("nobody", "hash"));
}

TEST_F(UserRepositoryTest, FindByID_ReturnsNulloptIfMissing)
{
    EXPECT_FALSE(m_repo->findByID(99999).has_value());
}

TEST_F(UserRepositoryTest, FindByUsername_ReturnsCorrectUser)
{
    m_repo->registerUser(make("eve"));
    ASSERT_TRUE(m_repo->findByUsername("eve").has_value());
}

TEST_F(UserRepositoryTest, FindByUsername_ReturnsNulloptIfMissing)
{
    EXPECT_FALSE(m_repo->findByUsername("nobody").has_value());
}

TEST_F(UserRepositoryTest, FindAll_ReturnsAllUsers)
{
    m_repo->registerUser(make("alice"));
    m_repo->registerUser(make("bob"));
    EXPECT_EQ(m_repo->findAll().size(), 2u);
}

TEST_F(UserRepositoryTest, FindAll_EmptyWhenNoUsers)
{
    EXPECT_TRUE(m_repo->findAll().empty());
}

TEST_F(UserRepositoryTest, Update_PersistsChanges)
{
    auto u = make("alice", "old"); m_repo->registerUser(u);
    u.username = "alice2"; u.password_hash = "new";
    EXPECT_TRUE(m_repo->update(u));
    auto f = m_repo->findByID(u.id.value());
    EXPECT_EQ(f->username,      "alice2");
    EXPECT_EQ(f->password_hash, "new");
}

TEST_F(UserRepositoryTest, Update_RejectsDuplicateUsername)
{
    auto u1 = make("alice"); m_repo->registerUser(u1);
    auto u2 = make("bob");   m_repo->registerUser(u2);
    u2.username = "alice";
    EXPECT_FALSE(m_repo->update(u2));
}

TEST_F(UserRepositoryTest, Update_FailsWithNoID)
{
    EXPECT_FALSE(m_repo->update(make()));
}

TEST_F(UserRepositoryTest, ChangePassword_PersistsNewHash)
{
    auto u = make("alice", "old"); m_repo->registerUser(u);
    EXPECT_TRUE(m_repo->changePassword(u.id.value(), "new"));
    EXPECT_EQ(m_repo->findByID(u.id.value())->password_hash, "new");
}

TEST_F(UserRepositoryTest, ChangePassword_NewHashWorksForAuthorize)
{
    auto u = make("alice", "old"); m_repo->registerUser(u);
    m_repo->changePassword(u.id.value(), "new");
    EXPECT_FALSE(m_repo->authorize("alice", "old"));
    EXPECT_TRUE(m_repo->authorize("alice", "new"));
}

TEST_F(UserRepositoryTest, ChangePassword_RejectsUnknownID)
{
    EXPECT_FALSE(m_repo->changePassword(99999, "new"));
}

TEST_F(UserRepositoryTest, HardDelete_RemovesUser)
{
    auto u = make(); m_repo->registerUser(u);
    int64_t id = u.id.value();
    EXPECT_TRUE(m_repo->hardDelete(id));
    EXPECT_FALSE(m_repo->findByID(id).has_value());
}

TEST_F(UserRepositoryTest, HardDelete_RejectsNonExistent)
{
    EXPECT_FALSE(m_repo->hardDelete(99999));
}