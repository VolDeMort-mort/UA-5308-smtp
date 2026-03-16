#include "TestFixture.h"
#include "../DAL/UserDAL.h"
#include "../DAL/FolderDAL.h"
#include "../DAL/MessageDAL.h"

class MessageDALTest : public DBFixture
{
protected:
    UserDAL*    userDal    = nullptr;
    FolderDAL*  folderDal  = nullptr;
    MessageDAL* messageDal = nullptr;

    int64_t userId   = 0;
    int64_t folderId = 0;

    void SetUp() override
    {
        DBFixture::SetUp();
        userDal    = new UserDAL(db);
        folderDal  = new FolderDAL(db);
        messageDal = new MessageDAL(db);

        User u;
        u.username = "alice"; u.password_hash = "h";
        ASSERT_TRUE(userDal->insert(u));
        userId = u.id.value();

        Folder f;
        f.user_id = userId; f.name = "INBOX";
        f.next_uid = 1; f.is_subscribed = true;
        ASSERT_TRUE(folderDal->insert(f));
        folderId = f.id.value();
    }

    void TearDown() override
    {
        delete messageDal;
        delete folderDal;
        delete userDal;
        DBFixture::TearDown();
    }

    Message makeMessage(const std::string& subject = "Hello", int64_t uid = 1)
    {
        Message m;
        m.user_id       = userId;
        m.folder_id     = folderId;
        m.uid           = uid;
        m.raw_file_path = "/store/msg" + std::to_string(uid) + ".eml";
        m.size_bytes    = 512;
        m.from_address  = "sender@example.com";
        m.subject       = subject;
        m.is_seen       = false;
        m.is_deleted    = false;
        m.is_draft      = false;
        m.is_answered   = false;
        m.is_flagged    = false;
        m.is_recent     = true;
        m.internal_date = "2024-01-01T00:00:00";
        return m;
    }

    Message insertMessage(const std::string& subject = "Hello", int64_t uid = 1)
    {
        Message m = makeMessage(subject, uid);
        EXPECT_TRUE(messageDal->insert(m));
        EXPECT_TRUE(m.id.has_value());
        return m;
    }
};

// ── insert ────────────────────────────────────────────────────────────────────

TEST_F(MessageDALTest, InsertSetsId)
{
    Message m = insertMessage();
    EXPECT_GT(m.id.value(), 0);
}

TEST_F(MessageDALTest, InsertPreservesFields)
{
    Message m = insertMessage("Test subject", 1);
    auto found = messageDal->findByID(m.id.value());

    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->subject.value_or(""), "Test subject");
    EXPECT_EQ(found->from_address, "sender@example.com");
    EXPECT_EQ(found->uid,          1);
    EXPECT_EQ(found->user_id,      userId);
    EXPECT_EQ(found->folder_id,    folderId);
    EXPECT_FALSE(found->is_seen);
    EXPECT_TRUE(found->is_recent);
}

TEST_F(MessageDALTest, InsertPreservesNullOptFields)
{
    Message m = makeMessage("sub", 1);
    m.mime_structure    = std::nullopt;
    m.sender_address    = std::nullopt;
    m.message_id_header = std::nullopt;
    ASSERT_TRUE(messageDal->insert(m));

    auto found = messageDal->findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_FALSE(found->mime_structure.has_value());
    EXPECT_FALSE(found->sender_address.has_value());
}

// ── findByID ──────────────────────────────────────────────────────────────────

TEST_F(MessageDALTest, FindByIDUnknownReturnsNullopt)
{
    EXPECT_FALSE(messageDal->findByID(9999).has_value());
}

// ── findByUID ─────────────────────────────────────────────────────────────────

TEST_F(MessageDALTest, FindByUIDReturnsCorrectMessage)
{
    Message m = insertMessage("Hello", 7);
    auto found = messageDal->findByUID(folderId, 7);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->id.value(), m.id.value());
}

TEST_F(MessageDALTest, FindByUIDWrongFolderReturnsNullopt)
{
    insertMessage("Hello", 1);

    Folder other;
    other.user_id = userId; other.name = "Sent";
    other.next_uid = 1; other.is_subscribed = true;
    ASSERT_TRUE(folderDal->insert(other));

    EXPECT_FALSE(messageDal->findByUID(other.id.value(), 1).has_value());
}

TEST_F(MessageDALTest, FindByUIDUnknownReturnsNullopt)
{
    EXPECT_FALSE(messageDal->findByUID(folderId, 9999).has_value());
}

// ── findByUser / findByFolder ─────────────────────────────────────────────────

TEST_F(MessageDALTest, FindByUserReturnsAllMessages)
{
    insertMessage("A", 1);
    insertMessage("B", 2);
    insertMessage("C", 3);
    EXPECT_EQ(messageDal->findByUser(userId, 100).size(), 3u);
}

TEST_F(MessageDALTest, FindByUserEmptyReturnsEmpty)
{
    EXPECT_TRUE(messageDal->findByUser(userId, 100).empty());
}

TEST_F(MessageDALTest, FindByUserLimitWorks)
{
    insertMessage("A", 1);
    insertMessage("B", 2);
    insertMessage("C", 3);
    EXPECT_EQ(messageDal->findByUser(userId, 2, 0).size(), 2u);
    EXPECT_EQ(messageDal->findByUser(userId, 2, 2).size(), 1u);
}

TEST_F(MessageDALTest, FindByFolderReturnsOnlyThatFolder)
{
    Folder other;
    other.user_id = userId; other.name = "Sent";
    other.next_uid = 1; other.is_subscribed = true;
    ASSERT_TRUE(folderDal->insert(other));

    insertMessage("inbox msg", 1);

    Message m2 = makeMessage("sent msg", 1);
    m2.folder_id = other.id.value();
    ASSERT_TRUE(messageDal->insert(m2));

    auto inboxMsgs = messageDal->findByFolder(folderId, 100);
    ASSERT_EQ(inboxMsgs.size(), 1u);
    EXPECT_EQ(inboxMsgs[0].subject.value_or(""), "inbox msg");
}

TEST_F(MessageDALTest, FindByFolderLimitWorks)
{
    insertMessage("A", 1);
    insertMessage("B", 2);
    insertMessage("C", 3);
    EXPECT_EQ(messageDal->findByFolder(folderId, 2, 0).size(), 2u);
    EXPECT_EQ(messageDal->findByFolder(folderId, 2, 2).size(), 1u);
}

// ── findUnseen / findDeleted / findFlagged ────────────────────────────────────

TEST_F(MessageDALTest, FindUnseenFiltersCorrectly)
{
    Message seen   = insertMessage("seen",   1);
    Message unseen = insertMessage("unseen", 2);

    messageDal->updateSeen(seen.id.value(), true);

    auto results = messageDal->findUnseen(folderId);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].subject.value_or(""), "unseen");
}

TEST_F(MessageDALTest, FindDeletedFiltersCorrectly)
{
    Message alive   = insertMessage("alive",   1);
    Message deleted = insertMessage("deleted", 2);

    messageDal->updateDeleted(deleted.id.value(), true);

    auto results = messageDal->findDeleted(folderId);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].subject.value_or(""), "deleted");
}

TEST_F(MessageDALTest, FindFlaggedFiltersCorrectly)
{
    Message normal  = insertMessage("normal",  1);
    Message flagged = insertMessage("flagged", 2);

    messageDal->updateFlags(flagged.id.value(),
                            false, false, false, false, true, false);

    auto results = messageDal->findFlagged(folderId, 100);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].subject.value_or(""), "flagged");
}

TEST_F(MessageDALTest, FindFlaggedLimitWorks)
{
    for (int i = 0; i < 3; ++i)
    {
        Message m = insertMessage("f" + std::to_string(i), i + 1);
        messageDal->updateFlags(m.id.value(), false, false, false, false, true, false);
    }
    EXPECT_EQ(messageDal->findFlagged(folderId, 2, 0).size(), 2u);
    EXPECT_EQ(messageDal->findFlagged(folderId, 2, 2).size(), 1u);
}

// ── search ────────────────────────────────────────────────────────────────────

TEST_F(MessageDALTest, SearchFindsMatchingSubject)
{
    insertMessage("Meeting tomorrow", 1);
    insertMessage("Lunch plans",      2);

    auto results = messageDal->search(userId, "Meeting", 100);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].subject.value_or(""), "Meeting tomorrow");
}

TEST_F(MessageDALTest, SearchReturnsEmptyWhenNoMatch)
{
    insertMessage("Hello world", 1);
    EXPECT_TRUE(messageDal->search(userId, "xyz_no_match", 100).empty());
}

TEST_F(MessageDALTest, SearchIsSubstringMatch)
{
    insertMessage("Re: Meeting tomorrow", 1);
    EXPECT_EQ(messageDal->search(userId, "Meeting", 100).size(), 1u);
}

TEST_F(MessageDALTest, SearchLimitWorks)
{
    for (int i = 0; i < 4; ++i)
        insertMessage("topic " + std::to_string(i), i + 1);

    EXPECT_EQ(messageDal->search(userId, "topic", 2, 0).size(), 2u);
    EXPECT_EQ(messageDal->search(userId, "topic", 2, 2).size(), 2u);
}

TEST_F(MessageDALTest, SearchWithLikeSpecialCharsDoesNotMatchAll)
{
    insertMessage("100% done",      1);
    insertMessage("something else", 2);

    // Without escaping, "%" builds pattern "%%" which matches everything (2 results).
    // With correct escaping, "%" is treated literally and only matches subjects
    // that actually contain the % character — so exactly 1 result.
    auto results = messageDal->search(userId, "%", 100);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].subject.value_or(""), "100% done");
}

// ── updateSeen / updateDeleted ────────────────────────────────────────────────

TEST_F(MessageDALTest, UpdateSeenToggles)
{
    Message m = insertMessage();

    ASSERT_TRUE(messageDal->updateSeen(m.id.value(), true));
    EXPECT_TRUE(messageDal->findByID(m.id.value())->is_seen);

    ASSERT_TRUE(messageDal->updateSeen(m.id.value(), false));
    EXPECT_FALSE(messageDal->findByID(m.id.value())->is_seen);
}

TEST_F(MessageDALTest, UpdateSeenNonExistentFails)
{
    EXPECT_FALSE(messageDal->updateSeen(9999, true));
}

TEST_F(MessageDALTest, UpdateDeletedToggles)
{
    Message m = insertMessage();

    ASSERT_TRUE(messageDal->updateDeleted(m.id.value(), true));
    EXPECT_TRUE(messageDal->findByID(m.id.value())->is_deleted);

    ASSERT_TRUE(messageDal->updateDeleted(m.id.value(), false));
    EXPECT_FALSE(messageDal->findByID(m.id.value())->is_deleted);
}

TEST_F(MessageDALTest, UpdateDeletedNonExistentFails)
{
    EXPECT_FALSE(messageDal->updateDeleted(9999, true));
}

// ── updateFlags ───────────────────────────────────────────────────────────────

TEST_F(MessageDALTest, UpdateFlagsSetsAllFlags)
{
    Message m = insertMessage();

    ASSERT_TRUE(messageDal->updateFlags(m.id.value(),
                                        true, true, true, true, true, true));

    auto found = messageDal->findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_TRUE(found->is_seen);
    EXPECT_TRUE(found->is_deleted);
    EXPECT_TRUE(found->is_draft);
    EXPECT_TRUE(found->is_answered);
    EXPECT_TRUE(found->is_flagged);
    EXPECT_TRUE(found->is_recent);
}

TEST_F(MessageDALTest, UpdateFlagsClearsAllFlags)
{
    Message m = insertMessage();
    messageDal->updateFlags(m.id.value(), true, true, true, true, true, true);

    ASSERT_TRUE(messageDal->updateFlags(m.id.value(),
                                        false, false, false, false, false, false));

    auto found = messageDal->findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_FALSE(found->is_seen);
    EXPECT_FALSE(found->is_deleted);
    EXPECT_FALSE(found->is_draft);
    EXPECT_FALSE(found->is_answered);
    EXPECT_FALSE(found->is_flagged);
    EXPECT_FALSE(found->is_recent);
}

TEST_F(MessageDALTest, UpdateFlagsNonExistentFails)
{
    EXPECT_FALSE(messageDal->updateFlags(9999,
                                         false, false, false, false, false, false));
}

// ── moveToFolder ──────────────────────────────────────────────────────────────

TEST_F(MessageDALTest, MoveToFolderChangesFolder)
{
    Folder dest;
    dest.user_id = userId; dest.name = "Archive";
    dest.next_uid = 1; dest.is_subscribed = true;
    ASSERT_TRUE(folderDal->insert(dest));

    Message m = insertMessage();
    ASSERT_TRUE(messageDal->moveToFolder(m.id.value(), dest.id.value(), 5));

    auto found = messageDal->findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->folder_id, dest.id.value());
    EXPECT_EQ(found->uid,       5);
}

TEST_F(MessageDALTest, MoveToFolderNonExistentMessageFails)
{
    EXPECT_FALSE(messageDal->moveToFolder(9999, folderId, 1));
}

// ── hardDelete ────────────────────────────────────────────────────────────────

TEST_F(MessageDALTest, HardDeleteRemovesMessage)
{
    Message m = insertMessage();
    ASSERT_TRUE(messageDal->hardDelete(m.id.value()));
    EXPECT_FALSE(messageDal->findByID(m.id.value()).has_value());
}

TEST_F(MessageDALTest, HardDeleteNonExistentFails)
{
    EXPECT_FALSE(messageDal->hardDelete(9999));
}

TEST_F(MessageDALTest, HardDeleteDoesNotAffectOtherMessages)
{
    Message a = insertMessage("A", 1);
    Message b = insertMessage("B", 2);
    ASSERT_TRUE(messageDal->hardDelete(a.id.value()));
    EXPECT_TRUE(messageDal->findByID(b.id.value()).has_value());
}

// ── update ────────────────────────────────────────────────────────────────────

TEST_F(MessageDALTest, UpdateChangesSubject)
{
    Message m = insertMessage("Original");
    m.subject = "Updated";
    ASSERT_TRUE(messageDal->update(m));

    auto found = messageDal->findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->subject.value_or(""), "Updated");
}

TEST_F(MessageDALTest, UpdateWithNoIdFails)
{
    Message m = makeMessage();
    EXPECT_FALSE(messageDal->update(m));
}

TEST_F(MessageDALTest, UpdateNonExistentFails)
{
    Message m = makeMessage();
    m.id = 9999;
    EXPECT_FALSE(messageDal->update(m));
}