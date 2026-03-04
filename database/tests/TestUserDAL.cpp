#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include <sqlite3.h>

#include "../DAL/UserDAL.h"
#include "../Entity/User.h"

class UserDALTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        sqlite3_open(":memory:", &m_db);

        std::ifstream file("scheme/001_init_scheme.sql");
        std::stringstream ss;
        ss << file.rdbuf();
        sqlite3_exec(m_db, ss.str().c_str(), nullptr, nullptr, nullptr);

        m_dal = new UserDAL(m_db);
    }

    void TearDown() override
    {
        delete m_dal;
        sqlite3_close(m_db);
    }

    User make(const std::string& username = "alice", const std::string& hash = "hash")
    {
        User u;
        u.username      = username;
        u.password_hash = hash;
        return u;
    }

    sqlite3* m_db  = nullptr;
    UserDAL* m_dal = nullptr;
};

TEST_F(UserDALTest, Insert_SetsID)
{
    auto u = make();
    EXPECT_TRUE(m_dal->insert(u));
    EXPECT_TRUE(u.id.has_value());
}

TEST_F(UserDALTest, Insert_PersistsFields)
{
    auto u = make("bob", "secret");
    m_dal->insert(u);

    auto f = m_dal->findByID(u.id.value());
    ASSERT_TRUE(f.has_value());
    EXPECT_EQ(f->username,      "bob");
    EXPECT_EQ(f->password_hash, "secret");
}

TEST_F(UserDALTest, Insert_RejectsDuplicateUsername)
{
    m_dal->insert(make("alice"));
    EXPECT_FALSE(m_dal->insert(make("alice")));
}

TEST_F(UserDALTest, FindByID_ReturnsNulloptIfMissing)
{
    EXPECT_FALSE(m_dal->findByID(99999).has_value());
}

TEST_F(UserDALTest, FindByUsername_ReturnsCorrectUser)
{
    auto u = make("charlie");
    m_dal->insert(u);

    auto f = m_dal->findByUsername("charlie");
    ASSERT_TRUE(f.has_value());
    EXPECT_EQ(f->username, "charlie");
}

TEST_F(UserDALTest, FindByUsername_ReturnsNulloptIfMissing)
{
    EXPECT_FALSE(m_dal->findByUsername("nobody").has_value());
}

TEST_F(UserDALTest, FindAll_ReturnsAllUsers)
{
    m_dal->insert(make("alice"));
    m_dal->insert(make("bob"));
    EXPECT_EQ(m_dal->findAll().size(), 2u);
}

TEST_F(UserDALTest, FindAll_EmptyWhenNoUsers)
{
    EXPECT_TRUE(m_dal->findAll().empty());
}

TEST_F(UserDALTest, Update_PersistsChanges)
{
    auto u = make("alice");
    m_dal->insert(u);
    u.username = "alice2";
    EXPECT_TRUE(m_dal->update(u));
    EXPECT_EQ(m_dal->findByID(u.id.value())->username, "alice2");
}

TEST_F(UserDALTest, Update_FailsWithNoID)
{
    auto u = make();
    EXPECT_FALSE(m_dal->update(u));
}

TEST_F(UserDALTest, Update_RejectsDuplicateUsername)
{
    auto u1 = make("alice"); m_dal->insert(u1);
    auto u2 = make("bob"); m_dal->insert(u2);
    u2.username = "alice";
    EXPECT_FALSE(m_dal->update(u2));
}

TEST_F(UserDALTest, UpdatePassword_PersistsNewHash)
{
    auto u = make("alice", "old");
    m_dal->insert(u);
    EXPECT_TRUE(m_dal->updatePassword(u.id.value(), "new"));
    EXPECT_EQ(m_dal->findByID(u.id.value())->password_hash, "new");
}

TEST_F(UserDALTest, HardDelete_RemovesUser)
{
    auto u = make(); m_dal->insert(u);
    int64_t id = u.id.value();
    EXPECT_TRUE(m_dal->hardDelete(id));
    EXPECT_FALSE(m_dal->findByID(id).has_value());
}

TEST_F(UserDALTest, HardDelete_CascadesToFolders)
{
    auto u = make(); m_dal->insert(u);

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(m_db, "INSERT INTO folders (user_id, name) VALUES (?, 'Inbox');", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, u.id.value());
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    m_dal->hardDelete(u.id.value());

    int count = 0;
    sqlite3_prepare_v2(m_db, "SELECT COUNT(*) FROM folders;", -1, &stmt, nullptr);
    if (sqlite3_step(stmt) == SQLITE_ROW) count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    EXPECT_EQ(count, 0);
}