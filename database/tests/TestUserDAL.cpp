#include "TestFixture.h"
#include "../DAL/UserDAL.h"

class UserDALTest : public DBFixture
{
protected:
    UserDAL* dal = nullptr;

    void SetUp() override
    {
        DBFixture::SetUp();
        dal = new UserDAL(db);
    }

    void TearDown() override
    {
        delete dal;
        DBFixture::TearDown();
    }

    User makeUser(const std::string& username, const std::string& hash)
    {
        User u;
        u.username      = username;
        u.password_hash = hash;
        EXPECT_TRUE(dal->insert(u));
        EXPECT_TRUE(u.id.has_value());
        return u;
    }
};

// ── insert ────────────────────────────────────────────────────────────────────

TEST_F(UserDALTest, InsertSetsId)
{
    User u;
    u.username      = "alice";
    u.password_hash = "hash_a";

    ASSERT_TRUE(dal->insert(u));
    EXPECT_TRUE(u.id.has_value());
    EXPECT_GT(u.id.value(), 0);
}

TEST_F(UserDALTest, InsertTwoUsersGetDifferentIds)
{
    User a = makeUser("alice", "hash_a");
    User b = makeUser("bob",   "hash_b");
    EXPECT_NE(a.id.value(), b.id.value());
}

TEST_F(UserDALTest, InsertDuplicateUsernameFails)
{
    makeUser("alice", "hash_a");

    User dup;
    dup.username      = "alice";
    dup.password_hash = "hash_other";
    EXPECT_FALSE(dal->insert(dup));
    EXPECT_FALSE(dal->getLastError().empty());
}

// ── findByID ──────────────────────────────────────────────────────────────────

TEST_F(UserDALTest, FindByIDReturnsUser)
{
    User inserted = makeUser("alice", "hash_a");
    auto found    = dal->findByID(inserted.id.value());

    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->username,      "alice");
    EXPECT_EQ(found->password_hash, "hash_a");
    EXPECT_EQ(found->id.value(),    inserted.id.value());
}

TEST_F(UserDALTest, FindByIDUnknownReturnsNullopt)
{
    EXPECT_FALSE(dal->findByID(9999).has_value());
}

// ── findByUsername ────────────────────────────────────────────────────────────

TEST_F(UserDALTest, FindByUsernameReturnsUser)
{
    makeUser("alice", "hash_a");
    auto found = dal->findByUsername("alice");

    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->password_hash, "hash_a");
}

TEST_F(UserDALTest, FindByUsernameUnknownReturnsNullopt)
{
    EXPECT_FALSE(dal->findByUsername("nobody").has_value());
}

TEST_F(UserDALTest, FindByUsernameIsCaseSensitive)
{
    makeUser("Alice", "hash_a");
    EXPECT_FALSE(dal->findByUsername("alice").has_value());
}

// ── findAll ───────────────────────────────────────────────────────────────────

TEST_F(UserDALTest, FindAllEmptyDatabase)
{
    EXPECT_TRUE(dal->findAll(100).empty());
}

TEST_F(UserDALTest, FindAllReturnsAllUsers)
{
    makeUser("charlie", "h");
    makeUser("alice",   "h");
    makeUser("bob",     "h");

    auto all = dal->findAll(100);
    ASSERT_EQ(all.size(), 3u);
    EXPECT_EQ(all[0].username, "alice");
    EXPECT_EQ(all[1].username, "bob");
    EXPECT_EQ(all[2].username, "charlie");
}

TEST_F(UserDALTest, FindAllLimitRestrictsResults)
{
    makeUser("alice",   "h");
    makeUser("bob",     "h");
    makeUser("charlie", "h");

    EXPECT_EQ(dal->findAll(2, 0).size(), 2u);
}

TEST_F(UserDALTest, FindAllOffsetSkipsRows)
{
    makeUser("alice",   "h");
    makeUser("bob",     "h");
    makeUser("charlie", "h");

    auto page2 = dal->findAll(10, 2);
    ASSERT_EQ(page2.size(), 1u);
    EXPECT_EQ(page2[0].username, "charlie");
}

// ── update ────────────────────────────────────────────────────────────────────

TEST_F(UserDALTest, UpdateChangesUsername)
{
    User u = makeUser("alice", "hash_a");
    u.username = "alice_renamed";

    ASSERT_TRUE(dal->update(u));

    auto found = dal->findByID(u.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->username, "alice_renamed");
}

TEST_F(UserDALTest, UpdateWithNoIdFails)
{
    User u;
    u.username      = "ghost";
    u.password_hash = "x";
    EXPECT_FALSE(dal->update(u));
    EXPECT_FALSE(dal->getLastError().empty());
}

TEST_F(UserDALTest, UpdateNonExistentIdFails)
{
    User u;
    u.id            = 9999;
    u.username      = "ghost";
    u.password_hash = "x";
    EXPECT_FALSE(dal->update(u));
}

// ── updatePassword ────────────────────────────────────────────────────────────

TEST_F(UserDALTest, UpdatePasswordChangesHash)
{
    User u = makeUser("alice", "old_hash");
    ASSERT_TRUE(dal->updatePassword(u.id.value(), "new_hash"));

    auto found = dal->findByID(u.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->password_hash, "new_hash");
}

TEST_F(UserDALTest, UpdatePasswordNonExistentIdFails)
{
    EXPECT_FALSE(dal->updatePassword(9999, "hash"));
}

// ── hardDelete ────────────────────────────────────────────────────────────────

TEST_F(UserDALTest, HardDeleteRemovesUser)
{
    User u = makeUser("alice", "hash_a");
    ASSERT_TRUE(dal->hardDelete(u.id.value()));
    EXPECT_FALSE(dal->findByID(u.id.value()).has_value());
}

TEST_F(UserDALTest, HardDeleteNonExistentFails)
{
    EXPECT_FALSE(dal->hardDelete(9999));
}

TEST_F(UserDALTest, HardDeleteDoesNotAffectOtherUsers)
{
    User a = makeUser("alice", "h");
    User b = makeUser("bob",   "h");

    ASSERT_TRUE(dal->hardDelete(a.id.value()));
    EXPECT_TRUE(dal->findByID(b.id.value()).has_value());
}