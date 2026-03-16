#include "TestFixture.h"
#include "../DAL/UserDAL.h"
#include "../DAL/FolderDAL.h"
#include "../DAL/MessageDAL.h"
#include "../DAL/RecipientDAL.h"
#include "../Repository/MessageRepository.h"

class MessageRepositoryTest : public DBFixture
{
protected:
    MessageRepository* repo = nullptr;

    int64_t userId   = 0;
    int64_t folderId = 0;

    void SetUp() override
    {
        DBFixture::SetUp();
        repo = new MessageRepository(db);

        // Seed a user and an INBOX folder directly via DAL
        UserDAL userDal(db);
        User u; u.username = "alice"; u.password_hash = "h";
        ASSERT_TRUE(userDal.insert(u));
        userId = u.id.value();

        Folder f; f.user_id = userId; f.name = "INBOX";
        f.next_uid = 1; f.is_subscribed = true;
        FolderDAL folderDal(db);
        ASSERT_TRUE(folderDal.insert(f));
        folderId = f.id.value();
    }

    void TearDown() override
    {
        delete repo;
        DBFixture::TearDown();
    }

    // Returns a minimal Message ready for deliver/saveToFolder
    Message makeMessage(const std::string& subject = "Hello", int64_t uid = 0)
    {
        Message m;
        m.user_id       = userId;
        m.folder_id     = folderId;
        m.uid           = uid;
        m.raw_file_path = "/msg.eml";
        m.size_bytes    = 256;
        m.from_address  = "sender@x.com";
        m.subject       = subject;
        m.is_seen = m.is_deleted = m.is_draft =
        m.is_answered = m.is_flagged = false;
        m.is_recent     = false;
        m.internal_date = "2024-01-01";
        return m;
    }

    // Creates a second folder (e.g. Sent, Trash) and returns its id
    int64_t makeFolder(const std::string& name)
    {
        Folder f; f.user_id = userId; f.name = name;
        f.next_uid = 1; f.is_subscribed = true;
        EXPECT_TRUE(repo->createFolder(f));
        return f.id.value();
    }
};

// ── deliver ───────────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, DeliverSetsCorrectFlags)
{
    Message m = makeMessage();
    ASSERT_TRUE(repo->deliver(m, folderId));

    auto found = repo->findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_FALSE(found->is_seen);
    EXPECT_TRUE(found->is_recent);
    EXPECT_FALSE(found->is_draft);
}

TEST_F(MessageRepositoryTest, DeliverAssignsUID)
{
    Message m = makeMessage();
    ASSERT_TRUE(repo->deliver(m, folderId));
    EXPECT_EQ(m.uid, 1);

    Message m2 = makeMessage("Second");
    ASSERT_TRUE(repo->deliver(m2, folderId));
    EXPECT_EQ(m2.uid, 2);
}

TEST_F(MessageRepositoryTest, DeliverIncrementsNextUID)
{
    Message m = makeMessage();
    repo->deliver(m, folderId);

    auto folder = repo->findFolderByID(folderId);
    ASSERT_TRUE(folder.has_value());
    EXPECT_EQ(folder->next_uid, 2);
}

TEST_F(MessageRepositoryTest, DeliverToNonExistentFolderFails)
{
    Message m = makeMessage();
    EXPECT_FALSE(repo->deliver(m, 9999));
}

// ── saveToFolder ──────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, SaveToFolderSetsCorrectFlags)
{
    Message m = makeMessage();
    ASSERT_TRUE(repo->saveToFolder(m, folderId));

    auto found = repo->findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_TRUE(found->is_seen);
    EXPECT_FALSE(found->is_recent);
}

// ── restoreToFolder ───────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, RestoreToFolderClearsDeletedFlag)
{
    // Deliver to INBOX, mark deleted (moved to trash), then restore
    Message m = makeMessage();
    ASSERT_TRUE(repo->deliver(m, folderId));
    repo->markDeleted(m.id.value(), true);
    repo->markSeen(m.id.value(), true);
    repo->markFlagged(m.id.value(), true);

    // Re-fetch to get current state
    auto current = repo->findByID(m.id.value());
    ASSERT_TRUE(current.has_value());

    int64_t inboxId = makeFolder("Restored");
    ASSERT_TRUE(repo->restoreToFolder(*current, inboxId));

    auto found = repo->findByID(current->id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_FALSE(found->is_deleted);   // cleared
    EXPECT_TRUE(found->is_seen);       // preserved
    EXPECT_TRUE(found->is_flagged);    // preserved
}

// ── mark* flags ───────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, MarkSeenToggles)
{
    Message m = makeMessage();
    repo->deliver(m, folderId);

    ASSERT_TRUE(repo->markSeen(m.id.value(), true));
    EXPECT_TRUE(repo->findByID(m.id.value())->is_seen);

    ASSERT_TRUE(repo->markSeen(m.id.value(), false));
    EXPECT_FALSE(repo->findByID(m.id.value())->is_seen);
}

TEST_F(MessageRepositoryTest, MarkDeletedToggles)
{
    Message m = makeMessage();
    repo->deliver(m, folderId);

    ASSERT_TRUE(repo->markDeleted(m.id.value(), true));
    EXPECT_TRUE(repo->findByID(m.id.value())->is_deleted);
}

TEST_F(MessageRepositoryTest, MarkFlaggedPreservesOtherFlags)
{
    Message m = makeMessage();
    repo->deliver(m, folderId);
    repo->markSeen(m.id.value(), true);

    ASSERT_TRUE(repo->markFlagged(m.id.value(), true));

    auto found = repo->findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_TRUE(found->is_flagged);
    EXPECT_TRUE(found->is_seen);   // unchanged
}

TEST_F(MessageRepositoryTest, MarkAnsweredPreservesOtherFlags)
{
    Message m = makeMessage();
    repo->deliver(m, folderId);
    repo->markFlagged(m.id.value(), true);

    ASSERT_TRUE(repo->markAnswered(m.id.value(), true));

    auto found = repo->findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_TRUE(found->is_answered);
    EXPECT_TRUE(found->is_flagged); // unchanged
}

TEST_F(MessageRepositoryTest, MarkDraftPreservesOtherFlags)
{
    Message m = makeMessage();
    repo->deliver(m, folderId);
    repo->markSeen(m.id.value(), true);

    ASSERT_TRUE(repo->markDraft(m.id.value(), true));

    auto found = repo->findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_TRUE(found->is_draft);
    EXPECT_TRUE(found->is_seen); // unchanged
}

TEST_F(MessageRepositoryTest, MarkFlaggedNonExistentFails)
{
    EXPECT_FALSE(repo->markFlagged(9999, true));
}

TEST_F(MessageRepositoryTest, MarkAnsweredNonExistentFails)
{
    EXPECT_FALSE(repo->markAnswered(9999, true));
}

TEST_F(MessageRepositoryTest, MarkDraftNonExistentFails)
{
    EXPECT_FALSE(repo->markDraft(9999, true));
}

// ── moveToFolder ──────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, MoveToFolderChangesFolder)
{
    Message m = makeMessage();
    ASSERT_TRUE(repo->deliver(m, folderId));

    int64_t archiveId = makeFolder("Archive");
    ASSERT_TRUE(repo->moveToFolder(m.id.value(), archiveId));

    auto found = repo->findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->folder_id, archiveId);
}

TEST_F(MessageRepositoryTest, MoveToFolderAssignsNewUID)
{
    Message m = makeMessage();
    ASSERT_TRUE(repo->deliver(m, folderId));

    int64_t archiveId = makeFolder("Archive");
    ASSERT_TRUE(repo->moveToFolder(m.id.value(), archiveId));

    auto found = repo->findByID(m.id.value());
    EXPECT_EQ(found->uid, 1); // Archive starts at uid=1
}

TEST_F(MessageRepositoryTest, MoveToFolderIncrementsDestNextUID)
{
    Message m = makeMessage();
    ASSERT_TRUE(repo->deliver(m, folderId));

    int64_t archiveId = makeFolder("Archive");
    repo->moveToFolder(m.id.value(), archiveId);

    auto folder = repo->findFolderByID(archiveId);
    EXPECT_EQ(folder->next_uid, 2);
}

TEST_F(MessageRepositoryTest, MoveToNonExistentFolderFails)
{
    Message m = makeMessage();
    repo->deliver(m, folderId);
    EXPECT_FALSE(repo->moveToFolder(m.id.value(), 9999));
}

// ── expunge ───────────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, ExpungeRemovesDeletedMessages)
{
    Message a = makeMessage("keep", 0);
    Message b = makeMessage("del",  0);
    repo->deliver(a, folderId);
    repo->deliver(b, folderId);
    repo->markDeleted(b.id.value(), true);

    ASSERT_TRUE(repo->expunge(folderId));

    EXPECT_TRUE(repo->findByID(a.id.value()).has_value());
    EXPECT_FALSE(repo->findByID(b.id.value()).has_value());
}

TEST_F(MessageRepositoryTest, ExpungeOnEmptyFolderSucceeds)
{
    EXPECT_TRUE(repo->expunge(folderId));
}

TEST_F(MessageRepositoryTest, ExpungeDoesNotRemoveNonDeletedMessages)
{
    Message m = makeMessage();
    repo->deliver(m, folderId);
    // is_deleted remains false

    repo->expunge(folderId);
    EXPECT_TRUE(repo->findByID(m.id.value()).has_value());
}

// ── hardDelete ────────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, HardDeleteRemovesMessage)
{
    Message m = makeMessage();
    repo->deliver(m, folderId);
    ASSERT_TRUE(repo->hardDelete(m.id.value()));
    EXPECT_FALSE(repo->findByID(m.id.value()).has_value());
}

TEST_F(MessageRepositoryTest, HardDeleteNonExistentFails)
{
    EXPECT_FALSE(repo->hardDelete(9999));
}

// ── createFolder ──────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, CreateFolderSucceeds)
{
    Folder f; f.user_id = userId; f.name = "Sent";
    f.next_uid = 1; f.is_subscribed = true;
    EXPECT_TRUE(repo->createFolder(f));
    EXPECT_TRUE(f.id.has_value());
}

TEST_F(MessageRepositoryTest, CreateFolderEmptyNameFails)
{
    Folder f; f.user_id = userId; f.name = "";
    f.next_uid = 1; f.is_subscribed = true;
    EXPECT_FALSE(repo->createFolder(f));
    EXPECT_FALSE(repo->getLastError().empty());
}

TEST_F(MessageRepositoryTest, CreateFolderDuplicateNameFails)
{
    Folder f; f.user_id = userId; f.name = "Sent";
    f.next_uid = 1; f.is_subscribed = true;
    ASSERT_TRUE(repo->createFolder(f));

    Folder dup; dup.user_id = userId; dup.name = "Sent";
    dup.next_uid = 1; dup.is_subscribed = true;
    EXPECT_FALSE(repo->createFolder(dup));
}

// ── renameFolder ──────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, RenameFolderSucceeds)
{
    int64_t id = makeFolder("OldName");
    ASSERT_TRUE(repo->renameFolder(id, "NewName"));

    auto found = repo->findFolderByID(id);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "NewName");
}

TEST_F(MessageRepositoryTest, RenameFolderEmptyNameFails)
{
    int64_t id = makeFolder("Test");
    EXPECT_FALSE(repo->renameFolder(id, ""));
}

TEST_F(MessageRepositoryTest, RenameFolderToExistingNameFails)
{
    int64_t a = makeFolder("Sent");
    int64_t b = makeFolder("Trash");
    EXPECT_FALSE(repo->renameFolder(a, "Trash"));
}

TEST_F(MessageRepositoryTest, RenameFolderNonExistentFails)
{
    EXPECT_FALSE(repo->renameFolder(9999, "X"));
}

// ── deleteFolder ──────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, DeleteFolderRemovesIt)
{
    int64_t id = makeFolder("Temp");
    ASSERT_TRUE(repo->deleteFolder(id));
    EXPECT_FALSE(repo->findFolderByID(id).has_value());
}

TEST_F(MessageRepositoryTest, DeleteFolderNonExistentFails)
{
    EXPECT_FALSE(repo->deleteFolder(9999));
}

// ── addRecipient / removeRecipient ────────────────────────────────────────────

TEST_F(MessageRepositoryTest, AddRecipientSucceeds)
{
    Message m = makeMessage();
    repo->deliver(m, folderId);

    Recipient r;
    r.message_id = m.id.value();
    r.address    = "bob@x.com";
    r.type       = Recipient::typeFromString("To");

    ASSERT_TRUE(repo->addRecipient(r));
    EXPECT_TRUE(r.id.has_value());
}

TEST_F(MessageRepositoryTest, AddRecipientEmptyAddressFails)
{
    Message m = makeMessage();
    repo->deliver(m, folderId);

    Recipient r;
    r.message_id = m.id.value();
    r.address    = "";
    r.type       = Recipient::typeFromString("To");
    EXPECT_FALSE(repo->addRecipient(r));
}

TEST_F(MessageRepositoryTest, AddRecipientNonExistentMessageFails)
{
    Recipient r;
    r.message_id = 9999;
    r.address    = "bob@x.com";
    r.type       = Recipient::typeFromString("To");
    EXPECT_FALSE(repo->addRecipient(r));
}

TEST_F(MessageRepositoryTest, RemoveRecipientSucceeds)
{
    Message m = makeMessage();
    repo->deliver(m, folderId);

    Recipient r;
    r.message_id = m.id.value();
    r.address    = "bob@x.com";
    r.type       = Recipient::typeFromString("To");
    ASSERT_TRUE(repo->addRecipient(r));

    ASSERT_TRUE(repo->removeRecipient(r.id.value()));
    EXPECT_FALSE(repo->findRecipientByID(r.id.value()).has_value());
}

TEST_F(MessageRepositoryTest, RemoveRecipientNonExistentFails)
{
    EXPECT_FALSE(repo->removeRecipient(9999));
}

// ── search ────────────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, SearchReturnsMatchingMessages)
{
    Message a = makeMessage("Project update");
    Message b = makeMessage("Lunch invite");
    repo->deliver(a, folderId);
    repo->deliver(b, folderId);

    auto results = repo->search(userId, "Project", 10);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].subject.value_or(""), "Project update");
}

TEST_F(MessageRepositoryTest, SearchWithLikeSpecialCharsDoesNotCrash)
{
    Message a = makeMessage("100% done");
    Message b = makeMessage("something else");
    repo->deliver(a, folderId);
    repo->deliver(b, folderId);

    // Without escaping, "%" builds pattern "%%" which matches everything (2 results).
    // With correct escaping, "%" is treated literally and only matches subjects
    // that actually contain the % character — so exactly 1 result.
    auto results = repo->search(userId, "%", 10);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].subject.value_or(""), "100% done");
}

TEST_F(MessageRepositoryTest, SearchPaginationWorks)
{
    for (int i = 0; i < 5; ++i)
    {
        Message m = makeMessage("topic " + std::to_string(i));
        repo->deliver(m, folderId);
    }

    EXPECT_EQ(repo->search(userId, "topic", 2, 0).size(), 2u);
    EXPECT_EQ(repo->search(userId, "topic", 2, 4).size(), 1u);
}

// ── findByFolder pagination ───────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, FindByFolderPaginationWorks)
{
    for (int i = 0; i < 4; ++i)
    {
        Message m = makeMessage("msg " + std::to_string(i));
        repo->deliver(m, folderId);
    }

    EXPECT_EQ(repo->findByFolder(folderId, 2, 0).size(), 2u);
    EXPECT_EQ(repo->findByFolder(folderId, 2, 2).size(), 2u);
    EXPECT_EQ(repo->findByFolder(folderId, 2, 4).size(), 0u);
}