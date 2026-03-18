#include "TestHelper.h"
#include "../DAL/UserDAL.h"

class UserDALTest : public DBFixture {};

// ── insert & findByID ────────────────────────────────────────────────────────

TEST_F(UserDALTest, InsertAssignsID)
{
    UserDAL dal(db);
    User u; u.username = "alice"; u.password_hash = "hash1";

    ASSERT_TRUE(dal.insert(u));
    ASSERT_TRUE(u.id.has_value());
    EXPECT_GT(u.id.value(), 0);
}

TEST_F(UserDALTest, FindByIDReturnsInsertedUser)
{
    UserDAL dal(db);
    User u; u.username = "bob"; u.password_hash = "hash2";
    ASSERT_TRUE(dal.insert(u));

    auto found = dal.findByID(u.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->username, "bob");
    EXPECT_EQ(found->password_hash, "hash2");
}

TEST_F(UserDALTest, FindByIDMissingReturnsNullopt)
{
    UserDAL dal(db);
    EXPECT_FALSE(dal.findByID(9999).has_value());
}

// ── findByUsername ───────────────────────────────────────────────────────────

TEST_F(UserDALTest, FindByUsernameSuccess)
{
    UserDAL dal(db);
    User u; u.username = "carol"; u.password_hash = "h";
    ASSERT_TRUE(dal.insert(u));

    auto found = dal.findByUsername("carol");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->id, u.id);
}

TEST_F(UserDALTest, FindByUsernameMissingReturnsNullopt)
{
    UserDAL dal(db);
    EXPECT_FALSE(dal.findByUsername("nobody").has_value());
}

TEST_F(UserDALTest, FindByUsernameIsCaseSensitive)
{
    UserDAL dal(db);
    User u; u.username = "Dave"; u.password_hash = "h";
    ASSERT_TRUE(dal.insert(u));

    EXPECT_FALSE(dal.findByUsername("dave").has_value());
}

// ── duplicate username ───────────────────────────────────────────────────────

TEST_F(UserDALTest, InsertDuplicateUsernameFails)
{
    UserDAL dal(db);
    User a; a.username = "dup"; a.password_hash = "h1";
    User b; b.username = "dup"; b.password_hash = "h2";

    ASSERT_TRUE(dal.insert(a));
    EXPECT_FALSE(dal.insert(b));
    EXPECT_FALSE(dal.getLastError().empty());
}

// ── update ───────────────────────────────────────────────────────────────────

TEST_F(UserDALTest, UpdateChangesUsername)
{
    UserDAL dal(db);
    User u; u.username = "old"; u.password_hash = "h";
    ASSERT_TRUE(dal.insert(u));

    u.username = "new";
    ASSERT_TRUE(dal.update(u));

    auto found = dal.findByID(u.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->username, "new");
}

TEST_F(UserDALTest, UpdateWithoutIDFails)
{
    UserDAL dal(db);
    User u; u.username = "x"; u.password_hash = "h";
    EXPECT_FALSE(dal.update(u));
}

// ── updatePassword ───────────────────────────────────────────────────────────

TEST_F(UserDALTest, UpdatePasswordPersists)
{
    UserDAL dal(db);
    User u; u.username = "eve"; u.password_hash = "old_hash";
    ASSERT_TRUE(dal.insert(u));

    ASSERT_TRUE(dal.updatePassword(u.id.value(), "new_hash"));

    auto found = dal.findByID(u.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->password_hash, "new_hash");
}

// ── hardDelete ───────────────────────────────────────────────────────────────

TEST_F(UserDALTest, HardDeleteRemovesUser)
{
    UserDAL dal(db);
    User u; u.username = "del"; u.password_hash = "h";
    ASSERT_TRUE(dal.insert(u));
    ASSERT_TRUE(dal.hardDelete(u.id.value()));
    EXPECT_FALSE(dal.findByID(u.id.value()).has_value());
}

TEST_F(UserDALTest, HardDeleteNonExistentSucceeds)
{
    // SQLite DELETE of non-existent row is still SQLITE_DONE
    UserDAL dal(db);
    EXPECT_TRUE(dal.hardDelete(9999));
}

// ── findAll + pagination ─────────────────────────────────────────────────────

TEST_F(UserDALTest, FindAllEmpty)
{
    UserDAL dal(db);
    EXPECT_TRUE(dal.findAll().empty());
}

TEST_F(UserDALTest, FindAllReturnsAllUsers)
{
    UserDAL dal(db);
    for (int i = 0; i < 5; ++i)
    {
        User u;
        u.username = "user" + std::to_string(i);
        u.password_hash = "h";
        ASSERT_TRUE(dal.insert(u));
    }
    EXPECT_EQ(dal.findAll(100, 0).size(), 5u);
}

TEST_F(UserDALTest, FindAllPaginationLimit)
{
    UserDAL dal(db);
    for (int i = 0; i < 10; ++i)
    {
        User u; u.username = "u" + std::to_string(i); u.password_hash = "h";
        ASSERT_TRUE(dal.insert(u));
    }
    EXPECT_EQ(dal.findAll(3, 0).size(), 3u);
}

TEST_F(UserDALTest, FindAllPaginationOffset)
{
    UserDAL dal(db);
    for (int i = 0; i < 10; ++i)
    {
        User u; u.username = "u" + std::to_string(i); u.password_hash = "h";
        ASSERT_TRUE(dal.insert(u));
    }
    // alphabetical order — offset 9 should give only 1
    EXPECT_EQ(dal.findAll(10, 9).size(), 1u);
}

TEST_F(UserDALTest, FindAllOffsetBeyondEndReturnsEmpty)
{
    UserDAL dal(db);
    User u; u.username = "only"; u.password_hash = "h";
    ASSERT_TRUE(dal.insert(u));
    EXPECT_TRUE(dal.findAll(10, 100).empty());
}

TEST_F(UserDALTest, FindAllOrderedAlphabetically)
{
    UserDAL dal(db);
    for (const char* name : {"zebra", "alpha", "mango"})
    {
        User u; u.username = name; u.password_hash = "h";
        ASSERT_TRUE(dal.insert(u));
    }
    auto users = dal.findAll(10, 0);
    ASSERT_EQ(users.size(), 3u);
    EXPECT_EQ(users[0].username, "alpha");
    EXPECT_EQ(users[1].username, "mango");
    EXPECT_EQ(users[2].username, "zebra");
}