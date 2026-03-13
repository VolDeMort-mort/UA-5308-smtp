#include "TestFixture.h"

struct MessageRepositoryTest : public DBFixture
{
    UserDAL           udal{nullptr};
    UserRepository    userRepo{udal};
    MessageRepository msgRepo{*static_cast<DataBaseManager*>(nullptr)};

    User   user;
    Folder inbox;
    Folder archive;

    void SetUp() override
    {
        DBFixture::SetUp();
        udal     = UserDAL(db);
        userRepo = UserRepository(udal);

        // MessageRepository takes a DataBaseManager, but we need the raw db.
        // Construct via the sqlite3* directly through a thin wrapper.
        new (&msgRepo) MessageRepository(db);

        user = makeUser(); userRepo.registerUser(user);
        inbox   = makeFolder(user.id.value(), "INBOX");
        archive = makeFolder(user.id.value(), "Archive");
        msgRepo.createFolder(inbox);
        msgRepo.createFolder(archive);
    }

    Message delivered()
    {
        Message m = makeMessage(user.id.value());
        msgRepo.deliver(m, inbox.id.value());
        return m;
    }
};

TEST_F(MessageRepositoryTest, DeliverAssignsUIDAndIncrementsCounter)
{
    Message m = makeMessage(user.id.value());
    EXPECT_TRUE(msgRepo.deliver(m, inbox.id.value()));
    EXPECT_TRUE(m.id.has_value());
    EXPECT_EQ(m.uid, static_cast<int64_t>(1));
    EXPECT_EQ(msgRepo.findFolderByID(inbox.id.value())->next_uid, static_cast<int64_t>(2));
}

TEST_F(MessageRepositoryTest, DeliverSetsCorrectFlags)
{
    Message m = delivered();
    auto found = msgRepo.findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_FALSE(found->is_seen);
    EXPECT_TRUE(found->is_recent);
}

TEST_F(MessageRepositoryTest, SaveToFolderSetsCorrectFlags)
{
    Message m = makeMessage(user.id.value());
    msgRepo.saveToFolder(m, inbox.id.value());
    auto found = msgRepo.findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_TRUE(found->is_seen);
    EXPECT_FALSE(found->is_recent);
}

TEST_F(MessageRepositoryTest, MultipleDeliversProduceSequentialUIDs)
{
    for (int i = 1; i <= 5; ++i)
    {
        Message m = makeMessage(user.id.value());
        msgRepo.deliver(m, inbox.id.value());
        EXPECT_EQ(m.uid, static_cast<int64_t>(i));
    }
}

TEST_F(MessageRepositoryTest, FindByUIDLocatesMessage)
{
    Message m = delivered();
    EXPECT_TRUE(msgRepo.findByUID(inbox.id.value(), 1).has_value());
}

TEST_F(MessageRepositoryTest, MarkSeenUpdatesFlag)
{
    Message m = delivered();
    msgRepo.markSeen(m.id.value(), true);
    EXPECT_TRUE(msgRepo.findByID(m.id.value())->is_seen);
}

TEST_F(MessageRepositoryTest, MarkDeletedUpdatesFlag)
{
    Message m = delivered();
    msgRepo.markDeleted(m.id.value(), true);
    EXPECT_TRUE(msgRepo.findByID(m.id.value())->is_deleted);
}

TEST_F(MessageRepositoryTest, MarkFlaggedUpdatesFlag)
{
    Message m = delivered();
    msgRepo.markFlagged(m.id.value(), true);
    EXPECT_TRUE(msgRepo.findByID(m.id.value())->is_flagged);
}

TEST_F(MessageRepositoryTest, MarkAnsweredUpdatesFlag)
{
    Message m = delivered();
    msgRepo.markAnswered(m.id.value(), true);
    EXPECT_TRUE(msgRepo.findByID(m.id.value())->is_answered);
}

TEST_F(MessageRepositoryTest, FindDeletedReturnsOnlyDeletedMessages)
{
    Message m1 = delivered();
    Message m2 = delivered();
    Message m3 = delivered();
    msgRepo.markDeleted(m1.id.value(), true);
    msgRepo.markDeleted(m3.id.value(), true);
    EXPECT_EQ(static_cast<int>(msgRepo.findDeleted(inbox.id.value()).size()), 2);
}

TEST_F(MessageRepositoryTest, ExpungeRemovesDeletedLeavesRest)
{
    Message m1 = delivered();
    Message m2 = delivered();
    Message m3 = delivered();
    msgRepo.markDeleted(m1.id.value(), true);
    msgRepo.markDeleted(m2.id.value(), true);
    EXPECT_TRUE(msgRepo.expunge(inbox.id.value()));
    EXPECT_FALSE(msgRepo.findByID(m1.id.value()).has_value());
    EXPECT_FALSE(msgRepo.findByID(m2.id.value()).has_value());
    EXPECT_TRUE (msgRepo.findByID(m3.id.value()).has_value());
}

TEST_F(MessageRepositoryTest, ExpungeOnEmptyFolderSucceeds)
{
    EXPECT_TRUE(msgRepo.expunge(inbox.id.value()));
}

TEST_F(MessageRepositoryTest, MoveToFolderAssignsNewUID)
{
    Message m = delivered();
    EXPECT_EQ(m.uid, static_cast<int64_t>(1));
    EXPECT_TRUE(msgRepo.moveToFolder(m.id.value(), archive.id.value()));
    auto found = msgRepo.findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->folder_id, archive.id.value());
    EXPECT_EQ(found->uid,       static_cast<int64_t>(1));
    EXPECT_EQ(msgRepo.findFolderByID(archive.id.value())->next_uid, static_cast<int64_t>(2));
}

TEST_F(MessageRepositoryTest, MoveToFolderFailsForNonexistentFolder)
{
    Message m = delivered();
    EXPECT_FALSE(msgRepo.moveToFolder(m.id.value(), 9999));
}

TEST_F(MessageRepositoryTest, CreateFolderRejectsEmptyName)
{
    Folder f; f.user_id = user.id.value(); f.name = "";
    EXPECT_FALSE(msgRepo.createFolder(f));
}

TEST_F(MessageRepositoryTest, CreateFolderRejectsDuplicateName)
{
    Folder f = makeFolder(user.id.value(), "INBOX");
    EXPECT_FALSE(msgRepo.createFolder(f));
}

TEST_F(MessageRepositoryTest, RenameFolderChangesName)
{
    EXPECT_TRUE(msgRepo.renameFolder(inbox.id.value(), "Primary"));
    EXPECT_EQ(msgRepo.findFolderByID(inbox.id.value())->name, std::string("Primary"));
}

TEST_F(MessageRepositoryTest, RenameFolderRejectsNameCollision)
{
    EXPECT_FALSE(msgRepo.renameFolder(inbox.id.value(), "Archive"));
}

TEST_F(MessageRepositoryTest, AddAndFindRecipients)
{
    Message m = delivered();
    Recipient r; r.message_id = m.id.value(); r.address = "bob@example.com"; r.type = RecipientType::To;
    EXPECT_TRUE(msgRepo.addRecipient(r));
    auto list = msgRepo.findRecipientsByMessage(m.id.value());
    ASSERT_EQ(static_cast<int>(list.size()), 1);
    EXPECT_EQ(list[0].address, std::string("bob@example.com"));
}

TEST_F(MessageRepositoryTest, AddRecipientFailsForNonexistentMessage)
{
    Recipient r; r.message_id = 9999; r.address = "x@x.com"; r.type = RecipientType::To;
    EXPECT_FALSE(msgRepo.addRecipient(r));
}

TEST_F(MessageRepositoryTest, RemoveRecipientDeletesIt)
{
    Message m = delivered();
    Recipient r; r.message_id = m.id.value(); r.address = "x@x.com"; r.type = RecipientType::To;
    msgRepo.addRecipient(r);
    EXPECT_TRUE(msgRepo.removeRecipient(r.id.value()));
    EXPECT_TRUE(msgRepo.findRecipientsByMessage(m.id.value()).empty());
}

TEST_F(MessageRepositoryTest, HardDeleteRemovesMessage)
{
    Message m = delivered();
    EXPECT_TRUE(msgRepo.hardDelete(m.id.value()));
    EXPECT_FALSE(msgRepo.findByID(m.id.value()).has_value());
}

TEST_F(MessageRepositoryTest, HardDeleteFailsForNonexistentMessage)
{
    EXPECT_FALSE(msgRepo.hardDelete(9999));
}