#include "TestFixture.h"

struct RecipientDALTest : public DBFixture
{
    UserDAL udal{nullptr};
    FolderDAL fdal{nullptr};
    MessageDAL mdal{nullptr};
    RecipientDAL rdal{nullptr};
    User user;
    Message msg;

    void SetUp() override
    {
        DBFixture::SetUp();
        udal = UserDAL(db);
        fdal = FolderDAL(db);
        mdal = MessageDAL(db);
        rdal = RecipientDAL(db);
        user = makeUser(); udal.insert(user);
        Folder f = makeFolder(user.id.value()); fdal.insert(f);
        msg = makeMessage(user.id.value());
        msg.folder_id = f.id.value(); msg.uid = 1;
        mdal.insert(msg);
    }

    Recipient makeR(const std::string& address, RecipientType type = RecipientType::To)
    {
        Recipient r;
        r.message_id = msg.id.value();
        r.address    = address;
        r.type       = type;
        return r;
    }
};

TEST_F(RecipientDALTest, InsertSetsID)
{
    Recipient r = makeR("to@x.com");
    EXPECT_TRUE(rdal.insert(r));
    EXPECT_TRUE(r.id.has_value());
}

TEST_F(RecipientDALTest, InsertPersistsFields)
{
    Recipient r = makeR("to@x.com", RecipientType::Cc);
    rdal.insert(r);
    auto found = rdal.findByID(r.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->address, std::string("to@x.com"));
    EXPECT_EQ(found->type, RecipientType::Cc);
    EXPECT_EQ(found->message_id, msg.id.value());
}

TEST_F(RecipientDALTest, FindByIDReturnsNulloptIfMissing)
{
    EXPECT_FALSE(rdal.findByID(9999).has_value());
}

TEST_F(RecipientDALTest, FindByMessageReturnsAll)
{
    rdal.insert(makeR("a@x.com", RecipientType::To));
    rdal.insert(makeR("b@x.com", RecipientType::Cc));
    rdal.insert(makeR("c@x.com", RecipientType::Bcc));
    EXPECT_EQ(static_cast<int>(rdal.findByMessage(msg.id.value()).size()), 3);
}

TEST_F(RecipientDALTest, FindByMessageEmptyWhenNone)
{
    EXPECT_TRUE(rdal.findByMessage(msg.id.value()).empty());
}

TEST_F(RecipientDALTest, AllTypeVariantsRoundTrip)
{
    rdal.insert(makeR("to@x.com",    RecipientType::To));
    rdal.insert(makeR("cc@x.com",    RecipientType::Cc));
    rdal.insert(makeR("bcc@x.com",   RecipientType::Bcc));
    rdal.insert(makeR("reply@x.com", RecipientType::ReplyTo));
    auto list = rdal.findByMessage(msg.id.value());
    ASSERT_EQ(static_cast<int>(list.size()), 4);
    EXPECT_EQ(list[0].type, RecipientType::To);
    EXPECT_EQ(list[1].type, RecipientType::Cc);
    EXPECT_EQ(list[2].type, RecipientType::Bcc);
    EXPECT_EQ(list[3].type, RecipientType::ReplyTo);
}

TEST_F(RecipientDALTest, UpdatePersistsChanges)
{
    Recipient r = makeR("old@x.com", RecipientType::To); rdal.insert(r);
    r.address = "new@x.com";
    r.type    = RecipientType::Cc;
    EXPECT_TRUE(rdal.update(r));
    auto found = rdal.findByID(r.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->address, std::string("new@x.com"));
    EXPECT_EQ(found->type, RecipientType::Cc);
}

TEST_F(RecipientDALTest, UpdateFailsWithNoID)
{
    Recipient r = makeR("x@x.com");
    EXPECT_FALSE(rdal.update(r));
}

TEST_F(RecipientDALTest, HardDeleteRemovesRecipient)
{
    Recipient r = makeR("x@x.com"); rdal.insert(r);
    EXPECT_TRUE(rdal.hardDelete(r.id.value()));
    EXPECT_FALSE(rdal.findByID(r.id.value()).has_value());
}

TEST_F(RecipientDALTest, HardDeleteOnlyRemovesTarget)
{
    Recipient r1 = makeR("a@x.com"); rdal.insert(r1);
    Recipient r2 = makeR("b@x.com"); rdal.insert(r2);
    rdal.hardDelete(r1.id.value());
    EXPECT_FALSE(rdal.findByID(r1.id.value()).has_value());
    EXPECT_TRUE(rdal.findByID(r2.id.value()).has_value());
}

TEST_F(RecipientDALTest, CascadeDeleteWithMessage)
{
    Recipient r = makeR("x@x.com"); rdal.insert(r);
    int64_t rid = r.id.value();
    mdal.hardDelete(msg.id.value());
    EXPECT_FALSE(rdal.findByID(rid).has_value());
}