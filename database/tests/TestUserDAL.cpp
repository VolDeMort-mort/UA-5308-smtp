#include "TestFixture.h"

struct UserDALTest : public DBFixture
{
    UserDAL dal{nullptr};
    void SetUp() override { DBFixture::SetUp(); dal = UserDAL(db); }
};

TEST_F(UserDALTest, InsertSetsID)
{
    User u = makeUser();
    EXPECT_TRUE(dal.insert(u));
    EXPECT_TRUE(u.id.has_value());
}

TEST_F(UserDALTest, FindByIDReturnsUser)
{
    User u = makeUser();
    dal.insert(u);
    auto found = dal.findByID(u.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->username, u.username);
}

TEST_F(UserDALTest, FindByUsernameReturnsUser)
{
    User u = makeUser("bob", "hash_bob");
    dal.insert(u);
    auto found = dal.findByUsername("bob");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->password_hash, std::string("hash_bob"));
}

TEST_F(UserDALTest, FindByUsernameReturnsNulloptForUnknown)
{
    EXPECT_FALSE(dal.findByUsername("nobody").has_value());
}

TEST_F(UserDALTest, DuplicateUsernameFailsInsert)
{
    User u1 = makeUser("dave");
    User u2 = makeUser("dave");
    dal.insert(u1);
    EXPECT_FALSE(dal.insert(u2));
}

TEST_F(UserDALTest, UpdatePasswordChangesHash)
{
    User u = makeUser();
    dal.insert(u);
    dal.updatePassword(u.id.value(), "new_hash");
    auto found = dal.findByID(u.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->password_hash, std::string("new_hash"));
}

TEST_F(UserDALTest, HardDeleteRemovesUser)
{
    User u = makeUser();
    dal.insert(u);
    dal.hardDelete(u.id.value());
    EXPECT_FALSE(dal.findByID(u.id.value()).has_value());
}

TEST_F(UserDALTest, FindAllReturnsAllUsers)
{
    User u1 = makeUser("user1"); dal.insert(u1);
    User u2 = makeUser("user2"); dal.insert(u2);
    User u3 = makeUser("user3"); dal.insert(u3);
    EXPECT_EQ(static_cast<int>(dal.findAll().size()), 3);
}