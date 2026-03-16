#include "TestFixture.h"
#include "../DAL/UserDAL.h"
#include "../DAL/FolderDAL.h"

class FolderDALTest : public DBFixture
{
protected:
    UserDAL*   userDal   = nullptr;
    FolderDAL* folderDal = nullptr;
    int64_t    userId    = 0;

    void SetUp() override
    {
        DBFixture::SetUp();
        userDal   = new UserDAL(db);
        folderDal = new FolderDAL(db);

        User u;
        u.username      = "alice";
        u.password_hash = "hash";
        ASSERT_TRUE(userDal->insert(u));
        userId = u.id.value();
    }

    void TearDown() override
    {
        delete folderDal;
        delete userDal;
        DBFixture::TearDown();
    }

    Folder makeFolder(const std::string& name, bool subscribed = true)
    {
        Folder f;
        f.user_id       = userId;
        f.name          = name;
        f.next_uid      = 1;
        f.is_subscribed = subscribed;
        EXPECT_TRUE(folderDal->insert(f));
        EXPECT_TRUE(f.id.has_value());
        return f;
    }
};

// ── insert ────────────────────────────────────────────────────────────────────

TEST_F(FolderDALTest, InsertSetsId)
{
    Folder f = makeFolder("INBOX");
    EXPECT_GT(f.id.value(), 0);
}

TEST_F(FolderDALTest, InsertPreservesFields)
{
    Folder f = makeFolder("INBOX");
    auto found = folderDal->findByID(f.id.value());

    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name,     "INBOX");
    EXPECT_EQ(found->user_id,  userId);
    EXPECT_EQ(found->next_uid, 1);
    EXPECT_TRUE(found->is_subscribed);
}

TEST_F(FolderDALTest, InsertTwoFoldersGetDifferentIds)
{
    Folder a = makeFolder("INBOX");
    Folder b = makeFolder("Sent");
    EXPECT_NE(a.id.value(), b.id.value());
}

// ── findByID ──────────────────────────────────────────────────────────────────

TEST_F(FolderDALTest, FindByIDReturnsFolder)
{
    Folder f = makeFolder("INBOX");
    auto found = folderDal->findByID(f.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "INBOX");
}

TEST_F(FolderDALTest, FindByIDUnknownReturnsNullopt)
{
    EXPECT_FALSE(folderDal->findByID(9999).has_value());
}

// ── findByUser ────────────────────────────────────────────────────────────────

TEST_F(FolderDALTest, FindByUserEmptyReturnsEmpty)
{
    EXPECT_TRUE(folderDal->findByUser(userId, 100).empty());
}

TEST_F(FolderDALTest, FindByUserReturnsFoldersOrderedByName)
{
    makeFolder("Trash");
    makeFolder("INBOX");
    makeFolder("Sent");

    auto folders = folderDal->findByUser(userId, 100);
    ASSERT_EQ(folders.size(), 3u);
    EXPECT_EQ(folders[0].name, "INBOX");
    EXPECT_EQ(folders[1].name, "Sent");
    EXPECT_EQ(folders[2].name, "Trash");
}

TEST_F(FolderDALTest, FindByUserDoesNotReturnOtherUsersFolders)
{
    User other;
    other.username      = "bob";
    other.password_hash = "h";
    ASSERT_TRUE(userDal->insert(other));

    Folder f;
    f.user_id       = other.id.value();
    f.name          = "INBOX";
    f.next_uid      = 1;
    f.is_subscribed = true;
    ASSERT_TRUE(folderDal->insert(f));

    EXPECT_TRUE(folderDal->findByUser(userId, 100).empty());
}

TEST_F(FolderDALTest, FindByUserLimitWorks)
{
    makeFolder("INBOX");
    makeFolder("Sent");
    makeFolder("Trash");

    EXPECT_EQ(folderDal->findByUser(userId, 2, 0).size(), 2u);
    EXPECT_EQ(folderDal->findByUser(userId, 2, 2).size(), 1u);
}

// ── findByName ────────────────────────────────────────────────────────────────

TEST_F(FolderDALTest, FindByNameReturnsCorrectFolder)
{
    makeFolder("INBOX");
    auto found = folderDal->findByName(userId, "INBOX");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "INBOX");
}

TEST_F(FolderDALTest, FindByNameUnknownReturnsNullopt)
{
    EXPECT_FALSE(folderDal->findByName(userId, "NoSuchFolder").has_value());
}

TEST_F(FolderDALTest, FindByNameIsScopedToUser)
{
    User other;
    other.username = "bob"; other.password_hash = "h";
    ASSERT_TRUE(userDal->insert(other));

    Folder f;
    f.user_id = other.id.value(); f.name = "INBOX";
    f.next_uid = 1; f.is_subscribed = true;
    ASSERT_TRUE(folderDal->insert(f));

    EXPECT_FALSE(folderDal->findByName(userId, "INBOX").has_value());
}

// ── update ────────────────────────────────────────────────────────────────────

TEST_F(FolderDALTest, UpdateChangesName)
{
    Folder f = makeFolder("INBOX");
    f.name = "INBOX_renamed";
    ASSERT_TRUE(folderDal->update(f));

    auto found = folderDal->findByID(f.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "INBOX_renamed");
}

TEST_F(FolderDALTest, UpdateWithNoIdFails)
{
    Folder f;
    f.user_id = userId; f.name = "X";
    f.next_uid = 1; f.is_subscribed = false;
    EXPECT_FALSE(folderDal->update(f));
}

TEST_F(FolderDALTest, UpdateNonExistentFails)
{
    Folder f;
    f.id = 9999; f.user_id = userId;
    f.name = "X"; f.next_uid = 1; f.is_subscribed = false;
    EXPECT_FALSE(folderDal->update(f));
}

// ── incrementNextUID ──────────────────────────────────────────────────────────

TEST_F(FolderDALTest, IncrementNextUIDIncreasesByOne)
{
    Folder f = makeFolder("INBOX");
    ASSERT_EQ(f.next_uid, 1);

    ASSERT_TRUE(folderDal->incrementNextUID(f.id.value()));

    auto found = folderDal->findByID(f.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->next_uid, 2);
}

TEST_F(FolderDALTest, IncrementNextUIDMultipleTimes)
{
    Folder f = makeFolder("INBOX");
    folderDal->incrementNextUID(f.id.value());
    folderDal->incrementNextUID(f.id.value());
    folderDal->incrementNextUID(f.id.value());

    auto found = folderDal->findByID(f.id.value());
    EXPECT_EQ(found->next_uid, 4);
}

TEST_F(FolderDALTest, IncrementNextUIDNonExistentFails)
{
    EXPECT_FALSE(folderDal->incrementNextUID(9999));
}

// ── hardDelete ────────────────────────────────────────────────────────────────

TEST_F(FolderDALTest, HardDeleteRemovesFolder)
{
    Folder f = makeFolder("INBOX");
    ASSERT_TRUE(folderDal->hardDelete(f.id.value()));
    EXPECT_FALSE(folderDal->findByID(f.id.value()).has_value());
}

TEST_F(FolderDALTest, HardDeleteNonExistentFails)
{
    EXPECT_FALSE(folderDal->hardDelete(9999));
}

TEST_F(FolderDALTest, HardDeleteDoesNotAffectOtherFolders)
{
    Folder a = makeFolder("INBOX");
    Folder b = makeFolder("Sent");
    ASSERT_TRUE(folderDal->hardDelete(a.id.value()));
    EXPECT_TRUE(folderDal->findByID(b.id.value()).has_value());
}