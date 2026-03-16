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

TEST_F(UserDALTest, InsertPersistsFields)
{
    User u = makeUser("alice", "hash_abc");
    dal.insert(u);
    auto found = dal.findByID(u.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->username, std::string("alice"));
    EXPECT_EQ(found->password_hash, std::string("hash_abc"));
}

TEST_F(UserDALTest, InsertRejectsDuplicateUsername)
{
    User u1 = makeUser("dave");
    User u2 = makeUser("dave");
    dal.insert(u1);
    EXPECT_FALSE(dal.insert(u2));
    EXPECT_FALSE(dal.getLastError().empty());
}

TEST_F(UserDALTest, FindByIDReturnsNulloptIfMissing)
{
    EXPECT_FALSE(dal.findByID(9999).has_value());
}

TEST_F(UserDALTest, FindByUsernameReturnsUser)
{
    User u = makeUser("bob", "hash_bob");
    dal.insert(u);
    auto found = dal.findByUsername("bob");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->password_hash, std::string("hash_bob"));
}

TEST_F(UserDALTest, FindByUsernameReturnsNulloptIfMissing)
{
    EXPECT_FALSE(dal.findByUsername("nobody").has_value());
}

TEST_F(UserDALTest, FindAllReturnsAllUsers)
{
    User u1 = makeUser("user1"); dal.insert(u1);
    User u2 = makeUser("user2"); dal.insert(u2);
    User u3 = makeUser("user3"); dal.insert(u3);
    EXPECT_EQ(static_cast<int>(dal.findAll().size()), 3);
}

TEST_F(UserDALTest, FindAllEmptyWhenNoUsers)
{
    EXPECT_TRUE(dal.findAll().empty());
}

TEST_F(UserDALTest, UpdatePersistsChanges)
{
    User u = makeUser("alice", "old"); dal.insert(u);
    u.username = "alice2";
    u.password_hash = "new";
    EXPECT_TRUE(dal.update(u));
    auto found = dal.findByID(u.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->username, std::string("alice2"));
    EXPECT_EQ(found->password_hash, std::string("new"));
}

TEST_F(UserDALTest, UpdateFailsWithNoID)
{
    User u = makeUser();
    EXPECT_FALSE(dal.update(u));
}

TEST_F(UserDALTest, UpdateRejectsDuplicateUsername)
{
    User u1 = makeUser("alice"); dal.insert(u1);
    User u2 = makeUser("bob");   dal.insert(u2);
    u2.username = "alice";
    EXPECT_FALSE(dal.update(u2));
}

TEST_F(UserDALTest, UpdatePasswordPersistsNewHash)
{
    User u = makeUser(); dal.insert(u);
    EXPECT_TRUE(dal.updatePassword(u.id.value(), "new_hash"));
    EXPECT_EQ(dal.findByID(u.id.value())->password_hash, std::string("new_hash"));
}

TEST_F(UserDALTest, HardDeleteRemovesUser)
{
    User u = makeUser(); dal.insert(u);
    EXPECT_TRUE(dal.hardDelete(u.id.value()));
    EXPECT_FALSE(dal.findByID(u.id.value()).has_value());
}

TEST_F(UserDALTest, HardDeleteCascadesToFolders)
{
    UserDAL udal(db); FolderDAL fdal(db);
    User u = makeUser(); udal.insert(u);
    Folder f = makeFolder(u.id.value()); fdal.insert(f);
    udal.hardDelete(u.id.value());
    EXPECT_FALSE(fdal.findByID(f.id.value()).has_value());
}