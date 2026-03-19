#include "TestHelper.h"
#include "../DAL/UserDAL.h"
#include "../Repository/MessageRepository.h"
#include "../Repository/UserRepository.h"

class MessageRepositoryTest : public DBFixture
{
protected:
    MessageRepository* repo     = nullptr;
    int64_t            user_id  = -1;
    int64_t            inbox_id = -1;
    int64_t            sent_id  = -1;

    void SetUp() override
    {
        DBFixture::SetUp();
        repo = new MessageRepository(db);

        UserDAL udal(db);
        User u; u.username = "tester"; u.password_hash = "h";
        ASSERT_TRUE(udal.insert(u));
        user_id = u.id.value();

        Folder inbox; inbox.user_id = user_id; inbox.name = "INBOX";
        inbox.next_uid = 1; inbox.is_subscribed = true;
        ASSERT_TRUE(repo->createFolder(inbox));
        inbox_id = inbox.id.value();

        Folder sent; sent.user_id = user_id; sent.name = "Sent";
        sent.next_uid = 1; sent.is_subscribed = true;
        ASSERT_TRUE(repo->createFolder(sent));
        sent_id = sent.id.value();
    }

    void TearDown() override
    {
        delete repo;
        DBFixture::TearDown();
    }

    Message makeMessage(const std::string& subject = "Test",
                        const std::string& from = "sender@example.com")
    {
        Message m;
        m.user_id       = user_id;
        m.folder_id     = inbox_id;
        m.uid           = 0;
        m.raw_file_path = "/tmp/test.eml";
        m.size_bytes    = 1024;
        m.from_address  = from;
        m.subject       = subject;
        m.internal_date = "2024-01-01T00:00:00Z";
        return m;
    }
};

// ── deliver ──────────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, DeliverSetsFlags)
{
    Message m = makeMessage();
    ASSERT_TRUE(repo->deliver(m, inbox_id));

    auto found = repo->findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_FALSE(found->is_seen);
    EXPECT_TRUE (found->is_recent);
    EXPECT_FALSE(found->is_draft);
}

TEST_F(MessageRepositoryTest, DeliverAssignsSequentialUIDs)
{
    Message m1 = makeMessage("First");
    Message m2 = makeMessage("Second");
    Message m3 = makeMessage("Third");
    ASSERT_TRUE(repo->deliver(m1, inbox_id));
    ASSERT_TRUE(repo->deliver(m2, inbox_id));
    ASSERT_TRUE(repo->deliver(m3, inbox_id));

    EXPECT_EQ(m1.uid, 1);
    EXPECT_EQ(m2.uid, 2);
    EXPECT_EQ(m3.uid, 3);
}

TEST_F(MessageRepositoryTest, DeliverIncrementsFolderNextUID)
{
    Message m = makeMessage();
    ASSERT_TRUE(repo->deliver(m, inbox_id));

    auto folder = repo->findFolderByID(inbox_id);
    ASSERT_TRUE(folder.has_value());
    EXPECT_EQ(folder->next_uid, 2);
}

TEST_F(MessageRepositoryTest, DeliverToNonExistentFolderFails)
{
    Message m = makeMessage();
    EXPECT_FALSE(repo->deliver(m, 9999));
    EXPECT_FALSE(repo->getLastError().empty());
}

// ── saveToFolder ─────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, SaveToFolderMarksSeen)
{
    Message m = makeMessage();
    ASSERT_TRUE(repo->saveToFolder(m, sent_id));

    auto found = repo->findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_TRUE(found->is_seen);
    EXPECT_FALSE(found->is_recent);
}

// ── findByFolder ─────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, FindByFolderPagination)
{
    for (int i = 0; i < 6; ++i)
    {
        Message m = makeMessage("msg" + std::to_string(i));
        ASSERT_TRUE(repo->deliver(m, inbox_id));
    }
    EXPECT_EQ(repo->findByFolder(inbox_id, 3, 0).size(), 3u);
    EXPECT_EQ(repo->findByFolder(inbox_id, 3, 3).size(), 3u);
    EXPECT_EQ(repo->findByFolder(inbox_id, 3, 6).size(), 0u);
}

// ── markSeen / markDeleted / markFlagged / markAnswered / markDraft ──────────

TEST_F(MessageRepositoryTest, MarkSeenUpdates)
{
    Message m = makeMessage(); ASSERT_TRUE(repo->deliver(m, inbox_id));
    ASSERT_TRUE(repo->markSeen(m.id.value(), true));
    EXPECT_TRUE(repo->findByID(m.id.value())->is_seen);
    ASSERT_TRUE(repo->markSeen(m.id.value(), false));
    EXPECT_FALSE(repo->findByID(m.id.value())->is_seen);
}

TEST_F(MessageRepositoryTest, MarkDeletedUpdates)
{
    Message m = makeMessage(); ASSERT_TRUE(repo->deliver(m, inbox_id));
    ASSERT_TRUE(repo->markDeleted(m.id.value(), true));
    EXPECT_TRUE(repo->findByID(m.id.value())->is_deleted);
}

TEST_F(MessageRepositoryTest, MarkFlaggedPreservesOtherFlags)
{
    Message m = makeMessage(); ASSERT_TRUE(repo->deliver(m, inbox_id));
    ASSERT_TRUE(repo->markSeen(m.id.value(), true));
    ASSERT_TRUE(repo->markFlagged(m.id.value(), true));

    auto found = repo->findByID(m.id.value());
    EXPECT_TRUE(found->is_seen);
    EXPECT_TRUE(found->is_flagged);
}

TEST_F(MessageRepositoryTest, MarkFlaggedNonExistentFails)
{
    EXPECT_FALSE(repo->markFlagged(9999, true));
}

TEST_F(MessageRepositoryTest, MarkAnsweredPreservesOtherFlags)
{
    Message m = makeMessage(); ASSERT_TRUE(repo->deliver(m, inbox_id));
    ASSERT_TRUE(repo->markAnswered(m.id.value(), true));
    EXPECT_TRUE(repo->findByID(m.id.value())->is_answered);
}

TEST_F(MessageRepositoryTest, MarkDraftPreservesOtherFlags)
{
    Message m = makeMessage(); ASSERT_TRUE(repo->deliver(m, inbox_id));
    ASSERT_TRUE(repo->markDraft(m.id.value(), true));
    EXPECT_TRUE(repo->findByID(m.id.value())->is_draft);
}

// ── setFlags (IMAP STORE) ────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, SetFlagsSetsSeen)
{
    Message m = makeMessage(); ASSERT_TRUE(repo->deliver(m, inbox_id));
    ASSERT_TRUE(repo->setFlags(m.id.value(), {"\\Seen"}));
    EXPECT_TRUE(repo->findByID(m.id.value())->is_seen);
}

TEST_F(MessageRepositoryTest, SetFlagsUnsetsWithDash)
{
    Message m = makeMessage(); ASSERT_TRUE(repo->deliver(m, inbox_id));
    ASSERT_TRUE(repo->markSeen(m.id.value(), true));
    ASSERT_TRUE(repo->setFlags(m.id.value(), {"-\\Seen"}));
    EXPECT_FALSE(repo->findByID(m.id.value())->is_seen);
}

TEST_F(MessageRepositoryTest, SetFlagsMultipleAtOnce)
{
    Message m = makeMessage(); ASSERT_TRUE(repo->deliver(m, inbox_id));
    ASSERT_TRUE(repo->setFlags(m.id.value(), {"\\Seen", "\\Flagged", "\\Answered"}));
    auto found = repo->findByID(m.id.value());
    EXPECT_TRUE(found->is_seen);
    EXPECT_TRUE(found->is_flagged);
    EXPECT_TRUE(found->is_answered);
}

TEST_F(MessageRepositoryTest, SetFlagsUnknownFlagFails)
{
    Message m = makeMessage(); ASSERT_TRUE(repo->deliver(m, inbox_id));
    EXPECT_FALSE(repo->setFlags(m.id.value(), {"\\NonExistent"}));
}

TEST_F(MessageRepositoryTest, SetFlagsNonExistentMessageFails)
{
    EXPECT_FALSE(repo->setFlags(9999, {"\\Seen"}));
}

// ── moveToFolder ─────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, MoveToFolderAssignsNewUID)
{
    Message m = makeMessage(); ASSERT_TRUE(repo->deliver(m, inbox_id));

    ASSERT_TRUE(repo->moveToFolder(m.id.value(), sent_id));

    auto found = repo->findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->folder_id, sent_id);
    EXPECT_EQ(found->uid, 1);
}

TEST_F(MessageRepositoryTest, MoveToFolderIncrementsSentNextUID)
{
    Message m = makeMessage(); ASSERT_TRUE(repo->deliver(m, inbox_id));
    ASSERT_TRUE(repo->moveToFolder(m.id.value(), sent_id));

    auto folder = repo->findFolderByID(sent_id);
    EXPECT_EQ(folder->next_uid, 2);
}

TEST_F(MessageRepositoryTest, MoveToNonExistentFolderFails)
{
    Message m = makeMessage(); ASSERT_TRUE(repo->deliver(m, inbox_id));
    EXPECT_FALSE(repo->moveToFolder(m.id.value(), 9999));
}

// ── expunge ──────────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, ExpungeRemovesAllDeletedMessages)
{
    for (int i = 0; i < 4; ++i)
    {
        Message m = makeMessage("msg" + std::to_string(i));
        ASSERT_TRUE(repo->deliver(m, inbox_id));
        if (i % 2 == 0)
            ASSERT_TRUE(repo->markDeleted(m.id.value(), true));
    }

    ASSERT_TRUE(repo->expunge(inbox_id));
    EXPECT_EQ(repo->findByFolder(inbox_id, INT_MAX, 0).size(), 2u);
    EXPECT_TRUE(repo->findDeleted(inbox_id, INT_MAX, 0).empty());
}

TEST_F(MessageRepositoryTest, ExpungeEmptyFolderSucceeds)
{
    EXPECT_TRUE(repo->expunge(inbox_id));
}

TEST_F(MessageRepositoryTest, ExpungeDoesNotTouchOtherFolders)
{
    Message m1 = makeMessage("inbox"); ASSERT_TRUE(repo->deliver(m1, inbox_id));
    ASSERT_TRUE(repo->markDeleted(m1.id.value(), true));

    Message m2 = makeMessage("sent"); ASSERT_TRUE(repo->deliver(m2, sent_id));
    ASSERT_TRUE(repo->markDeleted(m2.id.value(), true));

    ASSERT_TRUE(repo->expunge(inbox_id));

    EXPECT_TRUE(repo->findByFolder(inbox_id, 100, 0).empty());
    EXPECT_EQ  (repo->findByFolder(sent_id,  100, 0).size(), 1u);
}

// ── copy (IMAP COPY) ─────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, CopyCreatesNewMessageInTarget)
{
    Message m = makeMessage("Original"); ASSERT_TRUE(repo->deliver(m, inbox_id));

    auto copied = repo->copy(m.id.value(), sent_id);
    ASSERT_TRUE(copied.has_value());
    EXPECT_NE(copied->id, m.id);
    EXPECT_EQ(copied->folder_id, sent_id);
    EXPECT_EQ(copied->uid, 1);
}

TEST_F(MessageRepositoryTest, CopyPreservesSubject)
{
    Message m = makeMessage("Important"); ASSERT_TRUE(repo->deliver(m, inbox_id));

    auto copied = repo->copy(m.id.value(), sent_id);
    ASSERT_TRUE(copied.has_value());
    EXPECT_EQ(copied->subject.value(), "Important");
}

TEST_F(MessageRepositoryTest, CopyNonExistentMessageFails)
{
    EXPECT_FALSE(repo->copy(9999, sent_id).has_value());
}

TEST_F(MessageRepositoryTest, CopyToNonExistentFolderFails)
{
    Message m = makeMessage(); ASSERT_TRUE(repo->deliver(m, inbox_id));
    EXPECT_FALSE(repo->copy(m.id.value(), 9999).has_value());
}

// ── folder management ────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, CreateFolderSuccess)
{
    Folder f; f.user_id = user_id; f.name = "Archive"; f.next_uid = 1; f.is_subscribed = true;
    ASSERT_TRUE(repo->createFolder(f));
    ASSERT_TRUE(f.id.has_value());
    EXPECT_TRUE(repo->findFolderByName(user_id, "Archive").has_value());
}

TEST_F(MessageRepositoryTest, CreateFolderEmptyNameFails)
{
    Folder f; f.user_id = user_id; f.name = ""; f.next_uid = 1; f.is_subscribed = true;
    EXPECT_FALSE(repo->createFolder(f));
}

TEST_F(MessageRepositoryTest, CreateDuplicateFolderFails)
{
    Folder f; f.user_id = user_id; f.name = "INBOX"; f.next_uid = 1; f.is_subscribed = true;
    EXPECT_FALSE(repo->createFolder(f));
}

TEST_F(MessageRepositoryTest, RenameFolderSuccess)
{
    ASSERT_TRUE(repo->renameFolder(inbox_id, "NewInbox"));
    EXPECT_TRUE (repo->findFolderByName(user_id, "NewInbox").has_value());
    EXPECT_FALSE(repo->findFolderByName(user_id, "INBOX").has_value());
}

TEST_F(MessageRepositoryTest, RenameFolderToExistingNameFails)
{
    EXPECT_FALSE(repo->renameFolder(inbox_id, "Sent"));
}

TEST_F(MessageRepositoryTest, RenameFolderEmptyNameFails)
{
    EXPECT_FALSE(repo->renameFolder(inbox_id, ""));
}

TEST_F(MessageRepositoryTest, DeleteFolderSuccess)
{
    ASSERT_TRUE(repo->deleteFolder(sent_id));
    EXPECT_FALSE(repo->findFolderByID(sent_id).has_value());
}

TEST_F(MessageRepositoryTest, DeleteNonExistentFolderFails)
{
    EXPECT_FALSE(repo->deleteFolder(9999));
}

// ── recipients ───────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, AddAndFindRecipient)
{
    Message m = makeMessage(); ASSERT_TRUE(repo->deliver(m, inbox_id));

    Recipient r;
    r.message_id = m.id.value();
    r.address    = "to@x.com";
    r.type       = RecipientType::To;
    ASSERT_TRUE(repo->addRecipient(r));

    auto recipients = repo->findRecipientsByMessage(m.id.value());
    ASSERT_EQ(recipients.size(), 1u);
    EXPECT_EQ(recipients[0].address, "to@x.com");
}

TEST_F(MessageRepositoryTest, AddAllRecipientTypes)
{
    Message m = makeMessage(); ASSERT_TRUE(repo->deliver(m, inbox_id));

    for (RecipientType t : {RecipientType::To, RecipientType::Cc,
                            RecipientType::Bcc, RecipientType::ReplyTo})
    {
        Recipient r;
        r.message_id = m.id.value();
        r.address    = "addr@x.com";
        r.type       = t;
        ASSERT_TRUE(repo->addRecipient(r));
    }

    EXPECT_EQ(repo->findRecipientsByMessage(m.id.value()).size(), 4u);
}

TEST_F(MessageRepositoryTest, AddRecipientToNonExistentMessageFails)
{
    Recipient r; r.message_id = 9999; r.address = "x@x.com"; r.type = RecipientType::To;
    EXPECT_FALSE(repo->addRecipient(r));
}

TEST_F(MessageRepositoryTest, AddRecipientEmptyAddressFails)
{
    Message m = makeMessage(); ASSERT_TRUE(repo->deliver(m, inbox_id));
    Recipient r; r.message_id = m.id.value(); r.address = ""; r.type = RecipientType::To;
    EXPECT_FALSE(repo->addRecipient(r));
}

TEST_F(MessageRepositoryTest, RemoveRecipientSuccess)
{
    Message m = makeMessage(); ASSERT_TRUE(repo->deliver(m, inbox_id));
    Recipient r; r.message_id = m.id.value(); r.address = "del@x.com"; r.type = RecipientType::To;
    ASSERT_TRUE(repo->addRecipient(r));
    ASSERT_TRUE(repo->removeRecipient(r.id.value()));
    EXPECT_TRUE(repo->findRecipientsByMessage(m.id.value()).empty());
}

// ── hardDelete ───────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, HardDeleteRemovesMessage)
{
    Message m = makeMessage(); ASSERT_TRUE(repo->deliver(m, inbox_id));
    ASSERT_TRUE(repo->hardDelete(m.id.value()));
    EXPECT_FALSE(repo->findByID(m.id.value()).has_value());
}

TEST_F(MessageRepositoryTest, HardDeleteNonExistentFails)
{
    EXPECT_FALSE(repo->hardDelete(9999));
}

// ── search ───────────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, SearchFindsBySubject)
{
    Message m1 = makeMessage("Meeting notes"); ASSERT_TRUE(repo->deliver(m1, inbox_id));
    Message m2 = makeMessage("Lunch plans");   ASSERT_TRUE(repo->deliver(m2, inbox_id));

    auto results = repo->search(user_id, "Meeting", 100, 0);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].subject.value(), "Meeting notes");
}

TEST_F(MessageRepositoryTest, SearchPagination)
{
    for (int i = 0; i < 5; ++i)
    {
        Message m = makeMessage("Newsletter " + std::to_string(i));
        ASSERT_TRUE(repo->deliver(m, inbox_id));
    }
    EXPECT_EQ(repo->search(user_id, "Newsletter", 2, 0).size(), 2u);
    EXPECT_EQ(repo->search(user_id, "Newsletter", 2, 4).size(), 1u);
}