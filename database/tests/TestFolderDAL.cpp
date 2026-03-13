#include "TestFixture.h"

struct FolderDALTest : public DBFixture
{
    UserDAL   udal{nullptr};
    FolderDAL fdal{nullptr};
    User      user;

    void SetUp() override
    {
        DBFixture::SetUp();
        udal = UserDAL(db);
        fdal = FolderDAL(db);
        user = makeUser();
        udal.insert(user);
    }
};

TEST_F(FolderDALTest, InsertSetsIDAndDefaults)
{
    Folder f = makeFolder(user.id.value());
    EXPECT_TRUE(fdal.insert(f));
    EXPECT_TRUE(f.id.has_value());
    EXPECT_EQ(f.next_uid, static_cast<int64_t>(1));
    EXPECT_FALSE(f.is_subscribed);
}

TEST_F(FolderDALTest, FindByNameReturnsFolder)
{
    Folder f = makeFolder(user.id.value(), "Sent");
    fdal.insert(f);
    auto found = fdal.findByName(user.id.value(), "Sent");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, std::string("Sent"));
}

TEST_F(FolderDALTest, FindByUserReturnsOnlyThatUsersFolder)
{
    User u2 = makeUser("bob"); udal.insert(u2);

    Folder f1 = makeFolder(user.id.value(), "INBOX");   fdal.insert(f1);
    Folder f2 = makeFolder(user.id.value(), "Sent");    fdal.insert(f2);
    Folder f3 = makeFolder(u2.id.value(),   "INBOX");   fdal.insert(f3);

    EXPECT_EQ(static_cast<int>(fdal.findByUser(user.id.value()).size()), 2);
    EXPECT_EQ(static_cast<int>(fdal.findByUser(u2.id.value()).size()),   1);
}

TEST_F(FolderDALTest, IncrementNextUIDAdvancesCounter)
{
    Folder f = makeFolder(user.id.value()); fdal.insert(f);
    fdal.incrementNextUID(f.id.value());
    fdal.incrementNextUID(f.id.value());
    auto found = fdal.findByID(f.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->next_uid, static_cast<int64_t>(3));
}

TEST_F(FolderDALTest, UpdatePersistsChanges)
{
    Folder f = makeFolder(user.id.value(), "Old"); fdal.insert(f);
    f.name          = "New";
    f.is_subscribed = true;
    EXPECT_TRUE(fdal.update(f));
    auto found = fdal.findByID(f.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, std::string("New"));
    EXPECT_TRUE(found->is_subscribed);
}

TEST_F(FolderDALTest, HardDeleteRemovesFolder)
{
    Folder f = makeFolder(user.id.value()); fdal.insert(f);
    fdal.hardDelete(f.id.value());
    EXPECT_FALSE(fdal.findByID(f.id.value()).has_value());
}