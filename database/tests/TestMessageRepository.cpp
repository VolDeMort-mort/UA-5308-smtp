#include "TestFixture.h"

struct MessageRepositoryTest : public DBFixture
{
    UserDAL* udal = nullptr;
    UserRepository* userRepo = nullptr;
    MessageRepository* msgRepo = nullptr;

    User user;
    Folder inbox;
    Folder archive;

    void SetUp() override
    {
        DBFixture::SetUp();
        udal     = new UserDAL(db);
        userRepo = new UserRepository(*udal);
        msgRepo  = new MessageRepository(db);

        user = makeUser(); userRepo->registerUser(user);
        inbox   = makeFolder(user.id.value(), "INBOX");
        archive = makeFolder(user.id.value(), "Archive");
        msgRepo->createFolder(inbox);
        msgRepo->createFolder(archive);
    }

    void TearDown() override
    {
        delete msgRepo;
        delete userRepo;
        delete udal;
        DBFixture::TearDown();
    }

    Message delivered(int64_t folder_id = 0)
    {
        Message m = makeMessage(user.id.value());
        msgRepo->deliver(m, folder_id ? folder_id : inbox.id.value());
        return m;
    }
};

// --- deliver / saveToFolder ---

TEST_F(MessageRepositoryTest, DeliverAssignsUIDAndIncrementsCounter)
{
    Message m = makeMessage(user.id.value());
    EXPECT_TRUE(msgRepo->deliver(m, inbox.id.value()));
    EXPECT_TRUE(m.id.has_value());
    EXPECT_EQ(m.uid, static_cast<int64_t>(1));
    EXPECT_EQ(msgRepo->findFolderByID(inbox.id.value())->next_uid, static_cast<int64_t>(2));
}

TEST_F(MessageRepositoryTest, DeliverSetsCorrectFlags)
{
    Message m = delivered();
    auto found = msgRepo->findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_FALSE(found->is_seen);
    EXPECT_TRUE(found->is_recent);
    EXPECT_FALSE(found->is_draft);
}

TEST_F(MessageRepositoryTest, DeliverFailsForNonexistentFolder)
{
    Message m = makeMessage(user.id.value());
    EXPECT_FALSE(msgRepo->deliver(m, 9999));
}

TEST_F(MessageRepositoryTest, SaveToFolderSetsCorrectFlags)
{
    Message m = makeMessage(user.id.value());
    msgRepo->saveToFolder(m, inbox.id.value());
    auto found = msgRepo->findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_TRUE(found->is_seen);
    EXPECT_FALSE(found->is_recent);
}

TEST_F(MessageRepositoryTest, MultipleDeliversProduceSequentialUIDs)
{
    for (int i = 1; i <= 5; ++i)
    {
        Message m = makeMessage(user.id.value());
        msgRepo->deliver(m, inbox.id.value());
        EXPECT_EQ(m.uid, static_cast<int64_t>(i));
    }
}

TEST_F(MessageRepositoryTest, UIDsAreIndependentPerFolder)
{
    Message m1 = makeMessage(user.id.value()); msgRepo->deliver(m1, inbox.id.value());
    Message m2 = makeMessage(user.id.value()); msgRepo->deliver(m2, archive.id.value());
    EXPECT_EQ(m1.uid, static_cast<int64_t>(1));
    EXPECT_EQ(m2.uid, static_cast<int64_t>(1));
}

// --- find methods ---

TEST_F(MessageRepositoryTest, FindByIDReturnsNulloptIfMissing)
{
    EXPECT_FALSE(msgRepo->findByID(9999).has_value());
}

TEST_F(MessageRepositoryTest, FindByUIDLocatesMessage)
{
    Message m = delivered();
    EXPECT_TRUE(msgRepo->findByUID(inbox.id.value(), 1).has_value());
}

TEST_F(MessageRepositoryTest, FindByUIDReturnsNulloptIfMissing)
{
    EXPECT_FALSE(msgRepo->findByUID(inbox.id.value(), 99).has_value());
}

TEST_F(MessageRepositoryTest, FindByUserReturnsOnlyThatUser)
{
    User u2 = makeUser("bob"); userRepo->registerUser(u2);
    Folder f2 = makeFolder(u2.id.value(), "INBOX"); msgRepo->createFolder(f2);

    delivered();
    delivered();
    Message m3 = makeMessage(u2.id.value()); msgRepo->deliver(m3, f2.id.value());

    EXPECT_EQ(static_cast<int>(msgRepo->findByUser(user.id.value()).size()), 2);
    EXPECT_EQ(static_cast<int>(msgRepo->findByUser(u2.id.value()).size()), 1);
}

TEST_F(MessageRepositoryTest, FindByFolderReturnsOnlyThatFolder)
{
    delivered(inbox.id.value());
    delivered(inbox.id.value());
    delivered(archive.id.value());
    EXPECT_EQ(static_cast<int>(msgRepo->findByFolder(inbox.id.value()).size()), 2);
    EXPECT_EQ(static_cast<int>(msgRepo->findByFolder(archive.id.value()).size()), 1);
}

TEST_F(MessageRepositoryTest, FindUnseenReturnsOnlyUnseen)
{
    Message m1 = delivered(); msgRepo->markSeen(m1.id.value(), true);
    Message m2 = delivered();
    Message m3 = delivered();
    EXPECT_EQ(static_cast<int>(msgRepo->findUnseen(inbox.id.value()).size()), 2);
}

TEST_F(MessageRepositoryTest, FindDeletedReturnsOnlyDeleted)
{
    Message m1 = delivered();
    Message m2 = delivered();
    Message m3 = delivered();
    msgRepo->markDeleted(m1.id.value(), true);
    msgRepo->markDeleted(m3.id.value(), true);
    EXPECT_EQ(static_cast<int>(msgRepo->findDeleted(inbox.id.value()).size()), 2);
}

TEST_F(MessageRepositoryTest, FindFlaggedReturnsOnlyFlagged)
{
    Message m1 = delivered();
    Message m2 = delivered();
    msgRepo->markFlagged(m1.id.value(), true);
    EXPECT_EQ(static_cast<int>(msgRepo->findFlagged(inbox.id.value()).size()), 1);
}

TEST_F(MessageRepositoryTest, SearchFindsInSubject)
{
    Message m = makeMessage(user.id.value());
    m.subject = "Invoice Q3";
    msgRepo->deliver(m, inbox.id.value());
    EXPECT_EQ(static_cast<int>(msgRepo->search(user.id.value(), "Invoice").size()), 1);
}

TEST_F(MessageRepositoryTest, SearchDoesNotLeakAcrossUsers)
{
    User u2 = makeUser("bob"); userRepo->registerUser(u2);
    Folder f2 = makeFolder(u2.id.value(), "INBOX"); msgRepo->createFolder(f2);

    Message m1 = makeMessage(user.id.value()); m1.subject = "Secret"; msgRepo->deliver(m1, inbox.id.value());
    Message m2 = makeMessage(u2.id.value());   m2.subject = "Secret"; msgRepo->deliver(m2, f2.id.value());

    EXPECT_EQ(static_cast<int>(msgRepo->search(user.id.value(), "Secret").size()), 1);
}

// --- flags ---

TEST_F(MessageRepositoryTest, MarkSeenUpdatesFlag)
{
    Message m = delivered();
    msgRepo->markSeen(m.id.value(), true);
    EXPECT_TRUE(msgRepo->findByID(m.id.value())->is_seen);
    msgRepo->markSeen(m.id.value(), false);
    EXPECT_FALSE(msgRepo->findByID(m.id.value())->is_seen);
}

TEST_F(MessageRepositoryTest, MarkDeletedUpdatesFlag)
{
    Message m = delivered();
    msgRepo->markDeleted(m.id.value(), true);
    EXPECT_TRUE(msgRepo->findByID(m.id.value())->is_deleted);
}

TEST_F(MessageRepositoryTest, MarkFlaggedUpdatesFlag)
{
    Message m = delivered();
    msgRepo->markFlagged(m.id.value(), true);
    EXPECT_TRUE(msgRepo->findByID(m.id.value())->is_flagged);
}

TEST_F(MessageRepositoryTest, MarkAnsweredUpdatesFlag)
{
    Message m = delivered();
    msgRepo->markAnswered(m.id.value(), true);
    EXPECT_TRUE(msgRepo->findByID(m.id.value())->is_answered);
}

TEST_F(MessageRepositoryTest, MarkDraftUpdatesFlag)
{
    Message m = delivered();
    msgRepo->markDraft(m.id.value(), true);
    EXPECT_TRUE(msgRepo->findByID(m.id.value())->is_draft);
}

TEST_F(MessageRepositoryTest, MarkFlaggedFailsForNonexistentMessage)
{
    EXPECT_FALSE(msgRepo->markFlagged(9999, true));
}

TEST_F(MessageRepositoryTest, UpdateFlagsPersistsAllAtOnce)
{
    Message m = delivered();
    msgRepo->updateFlags(m.id.value(), true, true, true, true, true, false);
    auto found = msgRepo->findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_TRUE(found->is_seen);
    EXPECT_TRUE(found->is_deleted);
    EXPECT_TRUE(found->is_draft);
    EXPECT_TRUE(found->is_answered);
    EXPECT_TRUE(found->is_flagged);
    EXPECT_FALSE(found->is_recent);
}

// --- move / expunge ---

TEST_F(MessageRepositoryTest, MoveToFolderAssignsNewUID)
{
    Message m = delivered();
    EXPECT_EQ(m.uid, static_cast<int64_t>(1));
    EXPECT_TRUE(msgRepo->moveToFolder(m.id.value(), archive.id.value()));
    auto found = msgRepo->findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->folder_id, archive.id.value());
    EXPECT_EQ(found->uid, static_cast<int64_t>(1));
    EXPECT_EQ(msgRepo->findFolderByID(archive.id.value())->next_uid, static_cast<int64_t>(2));
}

TEST_F(MessageRepositoryTest, MoveToFolderSourceCounterUnchanged)
{
    delivered(); delivered();
    Message m3 = delivered();
    int64_t before = msgRepo->findFolderByID(inbox.id.value())->next_uid;
    msgRepo->moveToFolder(m3.id.value(), archive.id.value());
    EXPECT_EQ(msgRepo->findFolderByID(inbox.id.value())->next_uid, before);
}

TEST_F(MessageRepositoryTest, MoveToFolderFailsForNonexistentFolder)
{
    Message m = delivered();
    EXPECT_FALSE(msgRepo->moveToFolder(m.id.value(), 9999));
    EXPECT_FALSE(msgRepo->getLastError().empty());
}

TEST_F(MessageRepositoryTest, ExpungeRemovesDeletedLeavesRest)
{
    Message m1 = delivered();
    Message m2 = delivered();
    Message m3 = delivered();
    msgRepo->markDeleted(m1.id.value(), true);
    msgRepo->markDeleted(m2.id.value(), true);
    EXPECT_TRUE(msgRepo->expunge(inbox.id.value()));
    EXPECT_FALSE(msgRepo->findByID(m1.id.value()).has_value());
    EXPECT_FALSE(msgRepo->findByID(m2.id.value()).has_value());
    EXPECT_TRUE(msgRepo->findByID(m3.id.value()).has_value());
}

TEST_F(MessageRepositoryTest, ExpungeOnEmptyFolderSucceeds)
{
    EXPECT_TRUE(msgRepo->expunge(inbox.id.value()));
}

TEST_F(MessageRepositoryTest, ExpungeWithNoDeletedIsNoop)
{
    delivered(); delivered();
    EXPECT_TRUE(msgRepo->expunge(inbox.id.value()));
    EXPECT_EQ(static_cast<int>(msgRepo->findByFolder(inbox.id.value()).size()), 2);
}

// --- hardDelete ---

TEST_F(MessageRepositoryTest, HardDeleteRemovesMessage)
{
    Message m = delivered();
    EXPECT_TRUE(msgRepo->hardDelete(m.id.value()));
    EXPECT_FALSE(msgRepo->findByID(m.id.value()).has_value());
}

TEST_F(MessageRepositoryTest, HardDeleteFailsForNonexistentMessage)
{
    EXPECT_FALSE(msgRepo->hardDelete(9999));
    EXPECT_FALSE(msgRepo->getLastError().empty());
}

// --- folder management ---

TEST_F(MessageRepositoryTest, CreateFolderSetsID)
{
    Folder f = makeFolder(user.id.value(), "Drafts");
    EXPECT_TRUE(msgRepo->createFolder(f));
    EXPECT_TRUE(f.id.has_value());
}

TEST_F(MessageRepositoryTest, CreateFolderRejectsEmptyName)
{
    Folder f; f.user_id = user.id.value(); f.name = "";
    EXPECT_FALSE(msgRepo->createFolder(f));
}

TEST_F(MessageRepositoryTest, CreateFolderRejectsDuplicateName)
{
    Folder f = makeFolder(user.id.value(), "INBOX");
    EXPECT_FALSE(msgRepo->createFolder(f));
}

TEST_F(MessageRepositoryTest, FindFoldersByUserReturnsAll)
{
    auto folders = msgRepo->findFoldersByUser(user.id.value());
    EXPECT_EQ(static_cast<int>(folders.size()), 2);
}

TEST_F(MessageRepositoryTest, FindFolderByNameReturnsFolder)
{
    auto found = msgRepo->findFolderByName(user.id.value(), "INBOX");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, std::string("INBOX"));
}

TEST_F(MessageRepositoryTest, FindFolderByNameReturnsNulloptIfMissing)
{
    EXPECT_FALSE(msgRepo->findFolderByName(user.id.value(), "NoSuchFolder").has_value());
}

TEST_F(MessageRepositoryTest, RenameFolderChangesName)
{
    EXPECT_TRUE(msgRepo->renameFolder(inbox.id.value(), "Primary"));
    EXPECT_EQ(msgRepo->findFolderByID(inbox.id.value())->name, std::string("Primary"));
}

TEST_F(MessageRepositoryTest, RenameFolderRejectsEmptyName)
{
    EXPECT_FALSE(msgRepo->renameFolder(inbox.id.value(), ""));
}

TEST_F(MessageRepositoryTest, RenameFolderRejectsNameCollision)
{
    EXPECT_FALSE(msgRepo->renameFolder(inbox.id.value(), "Archive"));
}

TEST_F(MessageRepositoryTest, RenameFolderFailsForNonexistentFolder)
{
    EXPECT_FALSE(msgRepo->renameFolder(9999, "New"));
}

TEST_F(MessageRepositoryTest, DeleteFolderRemovesIt)
{
    EXPECT_TRUE(msgRepo->deleteFolder(inbox.id.value()));
    EXPECT_FALSE(msgRepo->findFolderByID(inbox.id.value()).has_value());
}

TEST_F(MessageRepositoryTest, DeleteFolderFailsForNonexistent)
{
    EXPECT_FALSE(msgRepo->deleteFolder(9999));
}

TEST_F(MessageRepositoryTest, DeleteFolderCascadesToMessages)
{
    Message m = delivered();
    msgRepo->deleteFolder(inbox.id.value());
    EXPECT_FALSE(msgRepo->findByID(m.id.value()).has_value());
}

// --- recipients ---

TEST_F(MessageRepositoryTest, AddAndFindRecipients)
{
    Message m = delivered();
    Recipient r; r.message_id = m.id.value(); r.address = "bob@x.com"; r.type = RecipientType::To;
    EXPECT_TRUE(msgRepo->addRecipient(r));
    EXPECT_TRUE(r.id.has_value());
    auto list = msgRepo->findRecipientsByMessage(m.id.value());
    ASSERT_EQ(static_cast<int>(list.size()), 1);
    EXPECT_EQ(list[0].address, std::string("bob@x.com"));
}

TEST_F(MessageRepositoryTest, AddMultipleRecipientsAllTypes)
{
    Message m = delivered();
    Recipient r1; r1.message_id = m.id.value(); r1.address = "to@x.com";    r1.type = RecipientType::To;
    Recipient r2; r2.message_id = m.id.value(); r2.address = "cc@x.com";    r2.type = RecipientType::Cc;
    Recipient r3; r3.message_id = m.id.value(); r3.address = "bcc@x.com";   r3.type = RecipientType::Bcc;
    Recipient r4; r4.message_id = m.id.value(); r4.address = "reply@x.com"; r4.type = RecipientType::ReplyTo;
    msgRepo->addRecipient(r1); msgRepo->addRecipient(r2);
    msgRepo->addRecipient(r3); msgRepo->addRecipient(r4);
    EXPECT_EQ(static_cast<int>(msgRepo->findRecipientsByMessage(m.id.value()).size()), 4);
}

TEST_F(MessageRepositoryTest, AddRecipientFailsForNonexistentMessage)
{
    Recipient r; r.message_id = 9999; r.address = "x@x.com"; r.type = RecipientType::To;
    EXPECT_FALSE(msgRepo->addRecipient(r));
}

TEST_F(MessageRepositoryTest, AddRecipientFailsForEmptyAddress)
{
    Message m = delivered();
    Recipient r; r.message_id = m.id.value(); r.address = ""; r.type = RecipientType::To;
    EXPECT_FALSE(msgRepo->addRecipient(r));
}

TEST_F(MessageRepositoryTest, RemoveRecipientDeletesIt)
{
    Message m = delivered();
    Recipient r; r.message_id = m.id.value(); r.address = "x@x.com"; r.type = RecipientType::To;
    msgRepo->addRecipient(r);
    EXPECT_TRUE(msgRepo->removeRecipient(r.id.value()));
    EXPECT_TRUE(msgRepo->findRecipientsByMessage(m.id.value()).empty());
}

TEST_F(MessageRepositoryTest, RemoveRecipientFailsForNonexistent)
{
    EXPECT_FALSE(msgRepo->removeRecipient(9999));
}