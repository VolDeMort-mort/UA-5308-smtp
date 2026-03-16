#include "TestFixture.h"

struct FolderDALTest : public DBFixture
{
    UserDAL udal{nullptr};
    FolderDAL fdal{nullptr};
    User user;

    void SetUp() override
    {
        DBFixture::SetUp();
        udal = UserDAL(db);
        fdal = FolderDAL(db);
        user = makeUser(); udal.insert(user);
    }
};

TEST_F(FolderDALTest, InsertSetsID)
{
    Folder f = makeFolder(user.id.value());
    EXPECT_TRUE(fdal.insert(f));
    EXPECT_TRUE(f.id.has_value());
}

TEST_F(FolderDALTest, InsertPersistsFields)
{
    Folder f = makeFolder(user.id.value(), "Sent"); fdal.insert(f);
    auto found = fdal.findByID(f.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, std::string("Sent"));
    EXPECT_EQ(found->user_id, user.id.value());
    EXPECT_EQ(found->next_uid, static_cast<int64_t>(1));
    EXPECT_FALSE(found->is_subscribed);
}

TEST_F(FolderDALTest, InsertRejectsUnknownUser)
{
    Folder f = makeFolder(9999);
    EXPECT_FALSE(fdal.insert(f));
}

TEST_F(FolderDALTest, InsertAllowsSameNameForDifferentUsers)
{
    User u2 = makeUser("bob"); udal.insert(u2);
    Folder f1 = makeFolder(user.id.value(), "INBOX"); fdal.insert(f1);
    Folder f2 = makeFolder(u2.id.value(),   "INBOX");
    EXPECT_TRUE(fdal.insert(f2));
}

TEST_F(FolderDALTest, FindByIDReturnsNulloptIfMissing)
{
    EXPECT_FALSE(fdal.findByID(9999).has_value());
}

TEST_F(FolderDALTest, FindByUserReturnsOnlyThatUsersFolder)
{
    User u2 = makeUser("bob"); udal.insert(u2);
    Folder f1 = makeFolder(user.id.value(), "INBOX"); fdal.insert(f1);
    Folder f2 = makeFolder(user.id.value(), "Sent");  fdal.insert(f2);
    Folder f3 = makeFolder(u2.id.value(),   "INBOX"); fdal.insert(f3);
    EXPECT_EQ(static_cast<int>(fdal.findByUser(user.id.value()).size()), 2);
    EXPECT_EQ(static_cast<int>(fdal.findByUser(u2.id.value()).size()), 1);
}

TEST_F(FolderDALTest, FindByUserEmptyWhenNone)
{
    EXPECT_TRUE(fdal.findByUser(user.id.value()).empty());
}

TEST_F(FolderDALTest, FindByNameReturnsCorrectFolder)
{
    Folder f = makeFolder(user.id.value(), "Drafts"); fdal.insert(f);
    auto found = fdal.findByName(user.id.value(), "Drafts");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, std::string("Drafts"));
}

TEST_F(FolderDALTest, FindByNameDoesNotReturnOtherUsersFolder)
{
    User u2 = makeUser("bob"); udal.insert(u2);
    Folder f = makeFolder(u2.id.value(), "INBOX"); fdal.insert(f);
    EXPECT_FALSE(fdal.findByName(user.id.value(), "INBOX").has_value());
}

TEST_F(FolderDALTest, UpdatePersistsNameChange)
{
    Folder f = makeFolder(user.id.value(), "Old"); fdal.insert(f);
    f.name = "New";
    EXPECT_TRUE(fdal.update(f));
    EXPECT_EQ(fdal.findByID(f.id.value())->name, std::string("New"));
}

TEST_F(FolderDALTest, UpdatePersistsIsSubscribed)
{
    Folder f = makeFolder(user.id.value()); fdal.insert(f);
    f.is_subscribed = true;
    fdal.update(f);
    EXPECT_TRUE(fdal.findByID(f.id.value())->is_subscribed);
}

TEST_F(FolderDALTest, UpdateFailsWithNoID)
{
    Folder f = makeFolder(user.id.value());
    EXPECT_FALSE(fdal.update(f));
}

TEST_F(FolderDALTest, IncrementNextUIDAdvancesCounter)
{
    Folder f = makeFolder(user.id.value()); fdal.insert(f);
    fdal.incrementNextUID(f.id.value());
    fdal.incrementNextUID(f.id.value());
    EXPECT_EQ(fdal.findByID(f.id.value())->next_uid, static_cast<int64_t>(3));
}

TEST_F(FolderDALTest, HardDeleteRemovesFolder)
{
    Folder f = makeFolder(user.id.value()); fdal.insert(f);
    EXPECT_TRUE(fdal.hardDelete(f.id.value()));
    EXPECT_FALSE(fdal.findByID(f.id.value()).has_value());
}

TEST_F(FolderDALTest, HardDeleteOnlyRemovesTarget)
{
    Folder f1 = makeFolder(user.id.value(), "INBOX"); fdal.insert(f1);
    Folder f2 = makeFolder(user.id.value(), "Sent");  fdal.insert(f2);
    fdal.hardDelete(f1.id.value());
    EXPECT_FALSE(fdal.findByID(f1.id.value()).has_value());
    EXPECT_TRUE(fdal.findByID(f2.id.value()).has_value());
}

TEST_F(FolderDALTest, CascadeDeletedWhenUserDeleted)
{
    Folder f = makeFolder(user.id.value()); fdal.insert(f);
    udal.hardDelete(user.id.value());
    EXPECT_FALSE(fdal.findByID(f.id.value()).has_value());
}