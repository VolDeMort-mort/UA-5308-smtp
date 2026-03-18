#include "TestHelper.h"
#include "../DAL/UserDAL.h"
#include "../DAL/FolderDAL.h"
#include "../DAL/MessageDAL.h"

class MessageDALTest : public DBFixture
{
protected:
    int64_t user_id   = -1;
    int64_t folder_id = -1;

    void SetUp() override
    {
        DBFixture::SetUp();

        UserDAL udal(db);
        User u; u.username = "tester"; u.password_hash = "h";
        ASSERT_TRUE(udal.insert(u));
        user_id = u.id.value();

        FolderDAL fdal(db);
        Folder f; f.user_id = user_id; f.name = "INBOX"; f.next_uid = 1; f.is_subscribed = true;
        ASSERT_TRUE(fdal.insert(f));
        folder_id = f.id.value();
    }

    Message makeMessage(int64_t uid = 1, const std::string& subject = "Test Subject")
    {
        Message m;
        m.user_id       = user_id;
        m.folder_id     = folder_id;
        m.uid           = uid;
        m.raw_file_path = "/mail/" + std::to_string(uid) + ".eml";
        m.size_bytes    = 1024;
        m.from_address  = "sender@example.com";
        m.subject       = subject;
        m.internal_date = "2024-01-01T00:00:00Z";
        m.is_seen       = false;
        m.is_deleted    = false;
        m.is_draft      = false;
        m.is_answered   = false;
        m.is_flagged    = false;
        m.is_recent     = true;
        return m;
    }
};

// ── insert & findByID ────────────────────────────────────────────────────────

TEST_F(MessageDALTest, InsertAssignsID)
{
    MessageDAL dal(db);
    Message m = makeMessage(1);
    ASSERT_TRUE(dal.insert(m));
    ASSERT_TRUE(m.id.has_value());
    EXPECT_GT(m.id.value(), 0);
}

TEST_F(MessageDALTest, FindByIDReturnsAllFields)
{
    MessageDAL dal(db);
    Message m = makeMessage(1, "Hello");
    m.message_id_header = "<abc@host>";
    m.in_reply_to       = "<prev@host>";
    m.sender_address    = "real@example.com";
    ASSERT_TRUE(dal.insert(m));

    auto found = dal.findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->subject.value(), "Hello");
    EXPECT_EQ(found->message_id_header.value(), "<abc@host>");
    EXPECT_EQ(found->sender_address.value(), "real@example.com");
    EXPECT_EQ(found->from_address, "sender@example.com");
    EXPECT_EQ(found->uid, 1);
    EXPECT_FALSE(found->is_seen);
    EXPECT_TRUE(found->is_recent);
}

TEST_F(MessageDALTest, FindByIDMissingReturnsNullopt)
{
    MessageDAL dal(db);
    EXPECT_FALSE(dal.findByID(9999).has_value());
}

// ── findByUID ────────────────────────────────────────────────────────────────

TEST_F(MessageDALTest, FindByUIDSuccess)
{
    MessageDAL dal(db);
    Message m = makeMessage(42);
    ASSERT_TRUE(dal.insert(m));

    auto found = dal.findByUID(folder_id, 42);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->id, m.id);
}

TEST_F(MessageDALTest, FindByUIDWrongFolderReturnsNullopt)
{
    MessageDAL dal(db);
    Message m = makeMessage(1);
    ASSERT_TRUE(dal.insert(m));
    EXPECT_FALSE(dal.findByUID(folder_id + 99, 1).has_value());
}

// ── findByFolder + pagination ────────────────────────────────────────────────

TEST_F(MessageDALTest, FindByFolderReturnsSortedByUID)
{
    MessageDAL dal(db);
    for (int uid : {3, 1, 2})
    {
        Message m = makeMessage(uid);
        ASSERT_TRUE(dal.insert(m));
    }
    auto msgs = dal.findByFolder(folder_id, 100, 0);
    ASSERT_EQ(msgs.size(), 3u);
    EXPECT_EQ(msgs[0].uid, 1);
    EXPECT_EQ(msgs[1].uid, 2);
    EXPECT_EQ(msgs[2].uid, 3);
}

TEST_F(MessageDALTest, FindByFolderPaginationLimit)
{
    MessageDAL dal(db);
    for (int i = 1; i <= 5; ++i) { Message m = makeMessage(i); ASSERT_TRUE(dal.insert(m)); }
    EXPECT_EQ(dal.findByFolder(folder_id, 2, 0).size(), 2u);
}

TEST_F(MessageDALTest, FindByFolderPaginationOffset)
{
    MessageDAL dal(db);
    for (int i = 1; i <= 5; ++i) { Message m = makeMessage(i); ASSERT_TRUE(dal.insert(m)); }
    auto page = dal.findByFolder(folder_id, 10, 3);
    ASSERT_EQ(page.size(), 2u);
    EXPECT_EQ(page[0].uid, 4);
}

// ── findUnseen ───────────────────────────────────────────────────────────────

TEST_F(MessageDALTest, FindUnseenReturnsOnlyUnseen)
{
    MessageDAL dal(db);
    Message seen    = makeMessage(1); seen.is_seen    = true;  ASSERT_TRUE(dal.insert(seen));
    Message unseen1 = makeMessage(2); unseen1.is_seen = false; ASSERT_TRUE(dal.insert(unseen1));
    Message unseen2 = makeMessage(3); unseen2.is_seen = false; ASSERT_TRUE(dal.insert(unseen2));

    auto result = dal.findUnseen(folder_id, 100, 0);
    EXPECT_EQ(result.size(), 2u);
    for (const auto& msg : result)
        EXPECT_FALSE(msg.is_seen);
}

TEST_F(MessageDALTest, FindUnseenPagination)
{
    MessageDAL dal(db);
    for (int i = 1; i <= 4; ++i) { Message m = makeMessage(i); ASSERT_TRUE(dal.insert(m)); }
    EXPECT_EQ(dal.findUnseen(folder_id, 2, 0).size(), 2u);
    EXPECT_EQ(dal.findUnseen(folder_id, 2, 2).size(), 2u);
}

// ── findDeleted ──────────────────────────────────────────────────────────────

TEST_F(MessageDALTest, FindDeletedReturnsOnlyDeleted)
{
    MessageDAL dal(db);
    Message alive   = makeMessage(1); alive.is_deleted   = false; ASSERT_TRUE(dal.insert(alive));
    Message deleted = makeMessage(2); deleted.is_deleted = true;  ASSERT_TRUE(dal.insert(deleted));

    auto result = dal.findDeleted(folder_id, 100, 0);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].uid, 2);
}

// ── findFlagged ──────────────────────────────────────────────────────────────

TEST_F(MessageDALTest, FindFlaggedReturnsOnlyFlagged)
{
    MessageDAL dal(db);
    Message normal  = makeMessage(1); normal.is_flagged  = false; ASSERT_TRUE(dal.insert(normal));
    Message flagged = makeMessage(2); flagged.is_flagged = true;  ASSERT_TRUE(dal.insert(flagged));

    auto result = dal.findFlagged(folder_id, 100, 0);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_TRUE(result[0].is_flagged);
}

// ── updateSeen ───────────────────────────────────────────────────────────────

TEST_F(MessageDALTest, UpdateSeenToggle)
{
    MessageDAL dal(db);
    Message m = makeMessage(1); m.is_seen = false;
    ASSERT_TRUE(dal.insert(m));

    ASSERT_TRUE(dal.updateSeen(m.id.value(), true));
    EXPECT_TRUE(dal.findByID(m.id.value())->is_seen);

    ASSERT_TRUE(dal.updateSeen(m.id.value(), false));
    EXPECT_FALSE(dal.findByID(m.id.value())->is_seen);
}

// ── updateDeleted ────────────────────────────────────────────────────────────

TEST_F(MessageDALTest, UpdateDeletedToggle)
{
    MessageDAL dal(db);
    Message m = makeMessage(1);
    ASSERT_TRUE(dal.insert(m));

    ASSERT_TRUE(dal.updateDeleted(m.id.value(), true));
    EXPECT_TRUE(dal.findByID(m.id.value())->is_deleted);

    ASSERT_TRUE(dal.updateDeleted(m.id.value(), false));
    EXPECT_FALSE(dal.findByID(m.id.value())->is_deleted);
}

// ── updateFlags ──────────────────────────────────────────────────────────────

TEST_F(MessageDALTest, UpdateFlagsPersistsAllFlags)
{
    MessageDAL dal(db);
    Message m = makeMessage(1);
    ASSERT_TRUE(dal.insert(m));

    ASSERT_TRUE(dal.updateFlags(m.id.value(), true, false, true, true, false, false));

    auto found = dal.findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_TRUE (found->is_seen);
    EXPECT_FALSE(found->is_deleted);
    EXPECT_TRUE (found->is_draft);
    EXPECT_TRUE (found->is_answered);
    EXPECT_FALSE(found->is_flagged);
    EXPECT_FALSE(found->is_recent);
}

// ── moveToFolder ─────────────────────────────────────────────────────────────

TEST_F(MessageDALTest, MoveToFolderUpdatesFields)
{
    FolderDAL fdal(db);
    Folder sent; sent.user_id = user_id; sent.name = "Sent"; sent.next_uid = 1; sent.is_subscribed = true;
    ASSERT_TRUE(fdal.insert(sent));

    MessageDAL dal(db);
    Message m = makeMessage(1);
    ASSERT_TRUE(dal.insert(m));

    ASSERT_TRUE(dal.moveToFolder(m.id.value(), sent.id.value(), 10));

    auto found = dal.findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->folder_id, sent.id.value());
    EXPECT_EQ(found->uid, 10);
}

// ── hardDelete ───────────────────────────────────────────────────────────────

TEST_F(MessageDALTest, HardDeleteRemovesMessage)
{
    MessageDAL dal(db);
    Message m = makeMessage(1);
    ASSERT_TRUE(dal.insert(m));
    ASSERT_TRUE(dal.hardDelete(m.id.value()));
    EXPECT_FALSE(dal.findByID(m.id.value()).has_value());
}

// ── search ───────────────────────────────────────────────────────────────────

TEST_F(MessageDALTest, SearchMatchesSubject)
{
    MessageDAL dal(db);
    Message m1 = makeMessage(1, "Meeting tomorrow");
    Message m2 = makeMessage(2, "Lunch plans");
    Message m3 = makeMessage(3, "RE: Meeting tomorrow");
    ASSERT_TRUE(dal.insert(m1));
    ASSERT_TRUE(dal.insert(m2));
    ASSERT_TRUE(dal.insert(m3));

    auto results = dal.search(user_id, "Meeting", 100, 0);
    EXPECT_EQ(results.size(), 2u);
}

TEST_F(MessageDALTest, SearchIsCaseInsensitive)
{
    MessageDAL dal(db);
    Message m = makeMessage(1, "Hello World");
    ASSERT_TRUE(dal.insert(m));

    // SQLite LIKE is case-insensitive for ASCII
    EXPECT_EQ(dal.search(user_id, "hello", 100, 0).size(), 1u);
}

TEST_F(MessageDALTest, SearchNoMatchReturnsEmpty)
{
    MessageDAL dal(db);
    Message m = makeMessage(1, "Unrelated");
    ASSERT_TRUE(dal.insert(m));
    EXPECT_TRUE(dal.search(user_id, "xyz_no_match", 100, 0).empty());
}

TEST_F(MessageDALTest, SearchPagination)
{
    MessageDAL dal(db);
    for (int i = 1; i <= 5; ++i)
    {
        Message m = makeMessage(i, "Promo offer " + std::to_string(i));
        ASSERT_TRUE(dal.insert(m));
    }
    EXPECT_EQ(dal.search(user_id, "Promo", 2, 0).size(), 2u);
    EXPECT_EQ(dal.search(user_id, "Promo", 2, 2).size(), 2u);
    EXPECT_EQ(dal.search(user_id, "Promo", 2, 4).size(), 1u);
}

// ── optional fields ──────────────────────────────────────────────────────────

TEST_F(MessageDALTest, NullOptionalFieldsRoundtrip)
{
    MessageDAL dal(db);
    Message m = makeMessage(1);
    m.subject          = std::nullopt;
    m.sender_address   = std::nullopt;
    m.mime_structure   = std::nullopt;
    m.message_id_header = std::nullopt;
    ASSERT_TRUE(dal.insert(m));

    auto found = dal.findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_FALSE(found->subject.has_value());
    EXPECT_FALSE(found->sender_address.has_value());
}