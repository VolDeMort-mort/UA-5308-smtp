#include "TestFixture.h"

struct MessageDALTest : public DBFixture
{
    UserDAL udal{nullptr};
    FolderDAL fdal{nullptr};
    MessageDAL mdal{nullptr};
    User user;
    Folder inbox;

    void SetUp() override
    {
        DBFixture::SetUp();
        udal = UserDAL(db);
        fdal = FolderDAL(db);
        mdal = MessageDAL(db);
        user  = makeUser(); udal.insert(user);
        inbox = makeFolder(user.id.value(), "INBOX"); fdal.insert(inbox);
    }

    Message inserted(int64_t uid = 1)
    {
        Message m = makeMessage(user.id.value());
        m.folder_id = inbox.id.value();
        m.uid = uid;
        mdal.insert(m);
        return m;
    }
};

TEST_F(MessageDALTest, InsertSetsID)
{
    Message m = makeMessage(user.id.value());
    m.folder_id = inbox.id.value(); m.uid = 1;
    EXPECT_TRUE(mdal.insert(m));
    EXPECT_TRUE(m.id.has_value());
}

TEST_F(MessageDALTest, InsertPersistsFields)
{
    Message m = makeMessage(user.id.value());
    m.folder_id    = inbox.id.value();
    m.uid          = 1;
    m.from_address = "from@x.com";
    m.subject      = "Test subject";
    m.size_bytes   = 512;
    m.is_draft     = true;
    mdal.insert(m);
    auto found = mdal.findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->from_address, std::string("from@x.com"));
    EXPECT_EQ(found->subject.value(), std::string("Test subject"));
    EXPECT_EQ(found->size_bytes, static_cast<int64_t>(512));
    EXPECT_TRUE(found->is_draft);
}

TEST_F(MessageDALTest, InsertRejectsUnknownUser)
{
    Message m = makeMessage(9999);
    m.folder_id = inbox.id.value(); m.uid = 1;
    EXPECT_FALSE(mdal.insert(m));
}

TEST_F(MessageDALTest, InsertRejectsUnknownFolder)
{
    Message m = makeMessage(user.id.value());
    m.folder_id = 9999; m.uid = 1;
    EXPECT_FALSE(mdal.insert(m));
}

TEST_F(MessageDALTest, InsertDuplicateFolderUIDIsRejected)
{
    Message m1 = makeMessage(user.id.value()); m1.folder_id = inbox.id.value(); m1.uid = 1; mdal.insert(m1);
    Message m2 = makeMessage(user.id.value()); m2.folder_id = inbox.id.value(); m2.uid = 1;
    EXPECT_FALSE(mdal.insert(m2));
}

TEST_F(MessageDALTest, FindByIDReturnsNulloptIfMissing)
{
    EXPECT_FALSE(mdal.findByID(9999).has_value());
}

TEST_F(MessageDALTest, FindByUIDReturnsCorrectMessage)
{
    inserted(1); inserted(2); inserted(3);
    auto found = mdal.findByUID(inbox.id.value(), 2);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->uid, static_cast<int64_t>(2));
}

TEST_F(MessageDALTest, FindByUIDReturnsNulloptIfMissing)
{
    EXPECT_FALSE(mdal.findByUID(inbox.id.value(), 99).has_value());
}

TEST_F(MessageDALTest, FindByUserReturnsOnlyThatUser)
{
    User u2 = makeUser("bob"); udal.insert(u2);
    Folder f2 = makeFolder(u2.id.value()); fdal.insert(f2);

    Message m1 = makeMessage(user.id.value()); m1.folder_id = inbox.id.value(); m1.uid = 1; mdal.insert(m1);
    Message m2 = makeMessage(user.id.value()); m2.folder_id = inbox.id.value(); m2.uid = 2; mdal.insert(m2);
    Message m3 = makeMessage(u2.id.value());   m3.folder_id = f2.id.value();    m3.uid = 1; mdal.insert(m3);

    EXPECT_EQ(static_cast<int>(mdal.findByUser(user.id.value()).size()), 2);
    EXPECT_EQ(static_cast<int>(mdal.findByUser(u2.id.value()).size()), 1);
}

TEST_F(MessageDALTest, FindByFolderReturnsOnlyThatFolder)
{
    Folder other = makeFolder(user.id.value(), "Sent"); fdal.insert(other);

    Message m1 = makeMessage(user.id.value()); m1.folder_id = inbox.id.value();  m1.uid = 1; mdal.insert(m1);
    Message m2 = makeMessage(user.id.value()); m2.folder_id = inbox.id.value();  m2.uid = 2; mdal.insert(m2);
    Message m3 = makeMessage(user.id.value()); m3.folder_id = other.id.value();  m3.uid = 1; mdal.insert(m3);

    EXPECT_EQ(static_cast<int>(mdal.findByFolder(inbox.id.value()).size()), 2);
    EXPECT_EQ(static_cast<int>(mdal.findByFolder(other.id.value()).size()), 1);
}

TEST_F(MessageDALTest, FindByFolderOrdersByUID)
{
    inserted(3); inserted(1); inserted(2);
    auto rows = mdal.findByFolder(inbox.id.value());
    ASSERT_EQ(static_cast<int>(rows.size()), 3);
    EXPECT_EQ(rows[0].uid, static_cast<int64_t>(1));
    EXPECT_EQ(rows[1].uid, static_cast<int64_t>(2));
    EXPECT_EQ(rows[2].uid, static_cast<int64_t>(3));
}

TEST_F(MessageDALTest, FindUnseenReturnsOnlyUnseen)
{
    Message m1 = makeMessage(user.id.value()); m1.folder_id = inbox.id.value(); m1.uid = 1; m1.is_seen = false; mdal.insert(m1);
    Message m2 = makeMessage(user.id.value()); m2.folder_id = inbox.id.value(); m2.uid = 2; m2.is_seen = true;  mdal.insert(m2);
    Message m3 = makeMessage(user.id.value()); m3.folder_id = inbox.id.value(); m3.uid = 3; m3.is_seen = false; mdal.insert(m3);
    EXPECT_EQ(static_cast<int>(mdal.findUnseen(inbox.id.value()).size()), 2);
}

TEST_F(MessageDALTest, FindDeletedReturnsOnlyDeleted)
{
    Message m1 = inserted(1);
    Message m2 = inserted(2);
    mdal.updateDeleted(m1.id.value(), true);
    EXPECT_EQ(static_cast<int>(mdal.findDeleted(inbox.id.value()).size()), 1);
}

TEST_F(MessageDALTest, FindFlaggedReturnsOnlyFlagged)
{
    Message m1 = inserted(1);
    Message m2 = inserted(2);
    mdal.updateFlags(m1.id.value(), false, false, false, false, true, false);
    EXPECT_EQ(static_cast<int>(mdal.findFlagged(inbox.id.value()).size()), 1);
}

TEST_F(MessageDALTest, SearchFindsInSubject)
{
    Message m = makeMessage(user.id.value());
    m.folder_id = inbox.id.value(); m.uid = 1; m.subject = "Quarterly Report";
    mdal.insert(m);
    EXPECT_EQ(static_cast<int>(mdal.search(user.id.value(), "Quarterly").size()), 1);
}

TEST_F(MessageDALTest, SearchNoMatchReturnsEmpty)
{
    inserted(1);
    EXPECT_TRUE(mdal.search(user.id.value(), "zzznomatch").empty());
}

TEST_F(MessageDALTest, SearchDoesNotLeakAcrossUsers)
{
    User u2 = makeUser("bob"); udal.insert(u2);
    Folder f2 = makeFolder(u2.id.value()); fdal.insert(f2);

    Message m1 = makeMessage(user.id.value()); m1.folder_id = inbox.id.value(); m1.uid = 1; m1.subject = "Keyword"; mdal.insert(m1);
    Message m2 = makeMessage(u2.id.value());   m2.folder_id = f2.id.value();    m2.uid = 1; m2.subject = "Keyword"; mdal.insert(m2);

    EXPECT_EQ(static_cast<int>(mdal.search(user.id.value(), "Keyword").size()), 1);
}

TEST_F(MessageDALTest, UpdatePersistsChanges)
{
    Message m = inserted();
    m.subject      = "Updated";
    m.from_address = "new@x.com";
    m.is_seen      = true;
    EXPECT_TRUE(mdal.update(m));
    auto found = mdal.findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->subject.value(), std::string("Updated"));
    EXPECT_EQ(found->from_address, std::string("new@x.com"));
    EXPECT_TRUE(found->is_seen);
}

TEST_F(MessageDALTest, UpdateFailsWithNoID)
{
    Message m = makeMessage(user.id.value());
    EXPECT_FALSE(mdal.update(m));
}

TEST_F(MessageDALTest, UpdateSeenToggles)
{
    Message m = inserted();
    EXPECT_TRUE(mdal.updateSeen(m.id.value(), true));
    EXPECT_TRUE(mdal.findByID(m.id.value())->is_seen);
    EXPECT_TRUE(mdal.updateSeen(m.id.value(), false));
    EXPECT_FALSE(mdal.findByID(m.id.value())->is_seen);
}

TEST_F(MessageDALTest, UpdateDeletedToggles)
{
    Message m = inserted();
    EXPECT_TRUE(mdal.updateDeleted(m.id.value(), true));
    EXPECT_TRUE(mdal.findByID(m.id.value())->is_deleted);
    EXPECT_TRUE(mdal.updateDeleted(m.id.value(), false));
    EXPECT_FALSE(mdal.findByID(m.id.value())->is_deleted);
}

TEST_F(MessageDALTest, UpdateFlagsPersistsAllFlags)
{
    Message m = inserted();
    mdal.updateFlags(m.id.value(), true, true, true, true, true, false);
    auto found = mdal.findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_TRUE(found->is_seen);
    EXPECT_TRUE(found->is_deleted);
    EXPECT_TRUE(found->is_draft);
    EXPECT_TRUE(found->is_answered);
    EXPECT_TRUE(found->is_flagged);
    EXPECT_FALSE(found->is_recent);
}

TEST_F(MessageDALTest, MoveToFolderUpdatesFields)
{
    Folder archive = makeFolder(user.id.value(), "Archive"); fdal.insert(archive);
    Message m = inserted(1);
    EXPECT_TRUE(mdal.moveToFolder(m.id.value(), archive.id.value(), 7));
    auto found = mdal.findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->folder_id, archive.id.value());
    EXPECT_EQ(found->uid, static_cast<int64_t>(7));
}

TEST_F(MessageDALTest, HardDeleteRemovesMessage)
{
    Message m = inserted();
    EXPECT_TRUE(mdal.hardDelete(m.id.value()));
    EXPECT_FALSE(mdal.findByID(m.id.value()).has_value());
}

TEST_F(MessageDALTest, HardDeleteOnlyRemovesTarget)
{
    Message m1 = inserted(1);
    Message m2 = inserted(2);
    mdal.hardDelete(m1.id.value());
    EXPECT_FALSE(mdal.findByID(m1.id.value()).has_value());
    EXPECT_TRUE(mdal.findByID(m2.id.value()).has_value());
}

TEST_F(MessageDALTest, CascadeDeletedWhenFolderDeleted)
{
    Message m = inserted();
    fdal.hardDelete(inbox.id.value());
    EXPECT_FALSE(mdal.findByID(m.id.value()).has_value());
}

TEST_F(MessageDALTest, CascadeDeletedWhenUserDeleted)
{
    Message m = inserted();
    udal.hardDelete(user.id.value());
    EXPECT_FALSE(mdal.findByID(m.id.value()).has_value());
}

TEST_F(MessageDALTest, OptionalEnvelopeFieldsRoundTrip)
{
    Message m = makeMessage(user.id.value());
    m.folder_id         = inbox.id.value();
    m.uid               = 1;
    m.message_id_header = "<abc@domain.com>";
    m.in_reply_to       = "<prev@domain.com>";
    m.references_header = "<root@domain.com> <prev@domain.com>";
    m.sender_address    = "proxy@domain.com";
    m.date_header       = "Mon, 1 Jan 2024 00:00:00 +0000";
    m.mime_structure    = "text/plain";
    mdal.insert(m);
    auto found = mdal.findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->message_id_header.value(), std::string("<abc@domain.com>"));
    EXPECT_EQ(found->in_reply_to.value(),       std::string("<prev@domain.com>"));
    EXPECT_EQ(found->references_header.value(), std::string("<root@domain.com> <prev@domain.com>"));
    EXPECT_EQ(found->sender_address.value(),    std::string("proxy@domain.com"));
    EXPECT_EQ(found->date_header.value(),       std::string("Mon, 1 Jan 2024 00:00:00 +0000"));
    EXPECT_EQ(found->mime_structure.value(),    std::string("text/plain"));
}

TEST_F(MessageDALTest, NullOptionalFieldsReturnNullopt)
{
    Message m = inserted();
    auto found = mdal.findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_FALSE(found->message_id_header.has_value());
    EXPECT_FALSE(found->in_reply_to.has_value());
    EXPECT_FALSE(found->references_header.has_value());
    EXPECT_FALSE(found->sender_address.has_value());
    EXPECT_FALSE(found->date_header.has_value());
    EXPECT_FALSE(found->mime_structure.has_value());
    EXPECT_FALSE(found->subject.has_value());
}