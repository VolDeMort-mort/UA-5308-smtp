#include "TestHelper.h"
#include "../DAL/UserDAL.h"
#include "../DAL/FolderDAL.h"

class FolderDALTest : public DBFixture
{
protected:
    int64_t user_id = -1;

    void SetUp() override
    {
        DBFixture::SetUp();
        UserDAL udal(db);
        User u; u.username = "owner"; u.password_hash = "h";
        ASSERT_TRUE(udal.insert(u));
        user_id = u.id.value();
    }

    Folder makeFolder(const std::string& name, bool subscribed = true)
    {
        Folder f;
        f.user_id       = user_id;
        f.name          = name;
        f.next_uid      = 1;
        f.is_subscribed = subscribed;
        return f;
    }
};

// ── insert & findByID ────────────────────────────────────────────────────────

TEST_F(FolderDALTest, InsertAssignsID)
{
    FolderDAL dal(db);
    Folder f = makeFolder("INBOX");
    ASSERT_TRUE(dal.insert(f));
    ASSERT_TRUE(f.id.has_value());
    EXPECT_GT(f.id.value(), 0);
}

TEST_F(FolderDALTest, FindByIDReturnsCorrectFolder)
{
    FolderDAL dal(db);
    Folder f = makeFolder("Sent");
    ASSERT_TRUE(dal.insert(f));

    auto found = dal.findByID(f.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "Sent");
    EXPECT_EQ(found->user_id, user_id);
    EXPECT_EQ(found->next_uid, 1);
    EXPECT_TRUE(found->is_subscribed);
}

TEST_F(FolderDALTest, FindByIDMissingReturnsNullopt)
{
    FolderDAL dal(db);
    EXPECT_FALSE(dal.findByID(9999).has_value());
}

// ── findByName ───────────────────────────────────────────────────────────────

TEST_F(FolderDALTest, FindByNameSuccess)
{
    FolderDAL dal(db);
    Folder f = makeFolder("Drafts");
    ASSERT_TRUE(dal.insert(f));

    auto found = dal.findByName(user_id, "Drafts");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->id, f.id);
}

TEST_F(FolderDALTest, FindByNameWrongUserReturnsNullopt)
{
    FolderDAL dal(db);
    Folder f = makeFolder("INBOX");
    ASSERT_TRUE(dal.insert(f));

    EXPECT_FALSE(dal.findByName(user_id + 99, "INBOX").has_value());
}

TEST_F(FolderDALTest, FindByNameMissingReturnsNullopt)
{
    FolderDAL dal(db);
    EXPECT_FALSE(dal.findByName(user_id, "NoSuchFolder").has_value());
}

// ── findByUser + pagination ──────────────────────────────────────────────────

TEST_F(FolderDALTest, FindByUserReturnsAllFolders)
{
    FolderDAL dal(db);
    for (const char* n : {"INBOX", "Sent", "Drafts", "Trash"})
    {
        Folder f = makeFolder(n);
        ASSERT_TRUE(dal.insert(f));
    }
    EXPECT_EQ(dal.findByUser(user_id, 100, 0).size(), 4u);
}

TEST_F(FolderDALTest, FindByUserOrderedAlphabetically)
{
    FolderDAL dal(db);
    for (const char* n : {"Trash", "INBOX", "Sent"})
    {
        Folder f = makeFolder(n);
        ASSERT_TRUE(dal.insert(f));
    }
    auto folders = dal.findByUser(user_id, 100, 0);
    ASSERT_EQ(folders.size(), 3u);
    EXPECT_EQ(folders[0].name, "INBOX");
    EXPECT_EQ(folders[1].name, "Sent");
    EXPECT_EQ(folders[2].name, "Trash");
}

TEST_F(FolderDALTest, FindByUserPaginationLimit)
{
    FolderDAL dal(db);
    for (int i = 0; i < 6; ++i)
    {
        Folder f = makeFolder("folder" + std::to_string(i));
        ASSERT_TRUE(dal.insert(f));
    }
    EXPECT_EQ(dal.findByUser(user_id, 2, 0).size(), 2u);
}

TEST_F(FolderDALTest, FindByUserPaginationOffset)
{
    FolderDAL dal(db);
    for (int i = 0; i < 4; ++i)
    {
        Folder f = makeFolder("f" + std::to_string(i));
        ASSERT_TRUE(dal.insert(f));
    }
    EXPECT_EQ(dal.findByUser(user_id, 10, 3).size(), 1u);
}

TEST_F(FolderDALTest, FindByUserDoesNotReturnOtherUsersfolders)
{
    UserDAL udal(db);
    User other; other.username = "other"; other.password_hash = "h";
    ASSERT_TRUE(udal.insert(other));

    FolderDAL dal(db);
    Folder f; f.user_id = other.id.value(); f.name = "INBOX"; f.next_uid = 1; f.is_subscribed = true;
    ASSERT_TRUE(dal.insert(f));

    EXPECT_TRUE(dal.findByUser(user_id, 100, 0).empty());
}

// ── update ───────────────────────────────────────────────────────────────────

TEST_F(FolderDALTest, UpdatePersistsChanges)
{
    FolderDAL dal(db);
    Folder f = makeFolder("Old");
    ASSERT_TRUE(dal.insert(f));

    f.name          = "New";
    f.is_subscribed = false;
    ASSERT_TRUE(dal.update(f));

    auto found = dal.findByID(f.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "New");
    EXPECT_FALSE(found->is_subscribed);
}

TEST_F(FolderDALTest, UpdateWithoutIDFails)
{
    FolderDAL dal(db);
    Folder f = makeFolder("X");
    EXPECT_FALSE(dal.update(f));
}

// ── incrementNextUID ─────────────────────────────────────────────────────────

TEST_F(FolderDALTest, IncrementNextUIDIncreasesByOne)
{
    FolderDAL dal(db);
    Folder f = makeFolder("INBOX");
    ASSERT_TRUE(dal.insert(f));

    ASSERT_TRUE(dal.incrementNextUID(f.id.value()));
    ASSERT_TRUE(dal.incrementNextUID(f.id.value()));

    auto found = dal.findByID(f.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->next_uid, 3);
}

// ── hardDelete ───────────────────────────────────────────────────────────────

TEST_F(FolderDALTest, HardDeleteRemovesFolder)
{
    FolderDAL dal(db);
    Folder f = makeFolder("ToDelete");
    ASSERT_TRUE(dal.insert(f));
    ASSERT_TRUE(dal.hardDelete(f.id.value()));
    EXPECT_FALSE(dal.findByID(f.id.value()).has_value());
}