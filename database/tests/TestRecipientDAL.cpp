#include "TestHelper.h"
#include "../DAL/UserDAL.h"
#include "../DAL/FolderDAL.h"
#include "../DAL/MessageDAL.h"
#include "../DAL/RecipientDAL.h"

class RecipientDALTest : public DBFixture
{
protected:
    int64_t message_id = -1;

    void SetUp() override
    {
        DBFixture::SetUp();

        UserDAL udal(db);
        User u; u.username = "owner"; u.password_hash = "h";
        ASSERT_TRUE(udal.insert(u));

        FolderDAL fdal(db);
        Folder f; f.user_id = u.id.value(); f.name = "INBOX"; f.next_uid = 1; f.is_subscribed = true;
        ASSERT_TRUE(fdal.insert(f));

        MessageDAL mdal(db);
        Message m;
        m.user_id = u.id.value(); m.folder_id = f.id.value(); m.uid = 1;
        m.raw_file_path = "/tmp/1.eml"; m.size_bytes = 512;
        m.from_address = "from@test.com"; m.internal_date = "2024-01-01T00:00:00Z";
        ASSERT_TRUE(mdal.insert(m));
        message_id = m.id.value();
    }

    Recipient makeRecipient(const std::string& address, RecipientType type = RecipientType::To)
    {
        Recipient r;
        r.message_id = message_id;
        r.address    = address;
        r.type       = type;
        return r;
    }
};

// ── insert & findByID ────────────────────────────────────────────────────────

TEST_F(RecipientDALTest, InsertAssignsID)
{
    RecipientDAL dal(db);
    Recipient r = makeRecipient("to@example.com");
    ASSERT_TRUE(dal.insert(r));
    ASSERT_TRUE(r.id.has_value());
}

TEST_F(RecipientDALTest, FindByIDReturnsCorrectFields)
{
    RecipientDAL dal(db);
    Recipient r = makeRecipient("cc@example.com", RecipientType::Cc);
    ASSERT_TRUE(dal.insert(r));

    auto found = dal.findByID(r.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->address, "cc@example.com");
    EXPECT_EQ(found->type, RecipientType::Cc);
    EXPECT_EQ(found->message_id, message_id);
}

TEST_F(RecipientDALTest, FindByIDMissingReturnsNullopt)
{
    RecipientDAL dal(db);
    EXPECT_FALSE(dal.findByID(9999).has_value());
}

// ── findByMessage ────────────────────────────────────────────────────────────

TEST_F(RecipientDALTest, FindByMessageReturnsAllRecipients)
{
    RecipientDAL dal(db);
    dal.insert(makeRecipient("to1@x.com",  RecipientType::To));
    dal.insert(makeRecipient("to2@x.com",  RecipientType::To));
    dal.insert(makeRecipient("cc1@x.com",  RecipientType::Cc));
    dal.insert(makeRecipient("bcc1@x.com", RecipientType::Bcc));

    auto result = dal.findByMessage(message_id);
    EXPECT_EQ(result.size(), 4u);
}

TEST_F(RecipientDALTest, FindByMessageEmptyWhenNone)
{
    RecipientDAL dal(db);
    EXPECT_TRUE(dal.findByMessage(message_id).empty());
}

TEST_F(RecipientDALTest, FindByMessageDoesNotReturnOtherMessages)
{
    UserDAL udal(db); FolderDAL fdal(db); MessageDAL mdal(db);
    User u2; u2.username = "u2"; u2.password_hash = "h"; udal.insert(u2);
    Folder f2; f2.user_id = u2.id.value(); f2.name = "INBOX"; f2.next_uid = 1; f2.is_subscribed = true;
    fdal.insert(f2);
    Message m2; m2.user_id = u2.id.value(); m2.folder_id = f2.id.value(); m2.uid = 1;
    m2.raw_file_path = "/tmp/2.eml"; m2.size_bytes = 100; m2.from_address = "x@y.com";
    m2.internal_date = "2024-01-01T00:00:00Z";
    mdal.insert(m2);

    RecipientDAL dal(db);
    Recipient r; r.message_id = m2.id.value(); r.address = "other@y.com"; r.type = RecipientType::To;
    dal.insert(r);

    EXPECT_TRUE(dal.findByMessage(message_id).empty());
}

// ── recipient types ──────────────────────────────────────────────────────────

TEST_F(RecipientDALTest, AllTypesRoundtrip)
{
    RecipientDAL dal(db);
    Recipient to      = makeRecipient("to@x.com",      RecipientType::To);
    Recipient cc      = makeRecipient("cc@x.com",      RecipientType::Cc);
    Recipient bcc     = makeRecipient("bcc@x.com",     RecipientType::Bcc);
    Recipient replyto = makeRecipient("reply@x.com",   RecipientType::ReplyTo);

    dal.insert(to); dal.insert(cc); dal.insert(bcc); dal.insert(replyto);

    auto all = dal.findByMessage(message_id);
    ASSERT_EQ(all.size(), 4u);

    int to_count = 0, cc_count = 0, bcc_count = 0, replyto_count = 0;
    for (const auto& r : all)
    {
        if (r.type == RecipientType::To)      ++to_count;
        if (r.type == RecipientType::Cc)      ++cc_count;
        if (r.type == RecipientType::Bcc)     ++bcc_count;
        if (r.type == RecipientType::ReplyTo) ++replyto_count;
    }
    EXPECT_EQ(to_count,      1);
    EXPECT_EQ(cc_count,      1);
    EXPECT_EQ(bcc_count,     1);
    EXPECT_EQ(replyto_count, 1);
}

// ── update ───────────────────────────────────────────────────────────────────

TEST_F(RecipientDALTest, UpdateChangesAddress)
{
    RecipientDAL dal(db);
    Recipient r = makeRecipient("old@x.com");
    ASSERT_TRUE(dal.insert(r));

    r.address = "new@x.com";
    ASSERT_TRUE(dal.update(r));

    auto found = dal.findByID(r.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->address, "new@x.com");
}

TEST_F(RecipientDALTest, UpdateChangesType)
{
    RecipientDAL dal(db);
    Recipient r = makeRecipient("x@x.com", RecipientType::To);
    ASSERT_TRUE(dal.insert(r));

    r.type = RecipientType::Cc;
    ASSERT_TRUE(dal.update(r));

    EXPECT_EQ(dal.findByID(r.id.value())->type, RecipientType::Cc);
}

TEST_F(RecipientDALTest, UpdateWithoutIDFails)
{
    RecipientDAL dal(db);
    Recipient r = makeRecipient("x@x.com");
    EXPECT_FALSE(dal.update(r));
}

// ── hardDelete ───────────────────────────────────────────────────────────────

TEST_F(RecipientDALTest, HardDeleteRemovesRecipient)
{
    RecipientDAL dal(db);
    Recipient r = makeRecipient("del@x.com");
    ASSERT_TRUE(dal.insert(r));
    ASSERT_TRUE(dal.hardDelete(r.id.value()));
    EXPECT_FALSE(dal.findByID(r.id.value()).has_value());
}

TEST_F(RecipientDALTest, HardDeleteOnlyRemovesTargeted)
{
    RecipientDAL dal(db);
    Recipient r1 = makeRecipient("keep@x.com");
    Recipient r2 = makeRecipient("del@x.com");
    ASSERT_TRUE(dal.insert(r1));
    ASSERT_TRUE(dal.insert(r2));

    ASSERT_TRUE(dal.hardDelete(r2.id.value()));

    EXPECT_TRUE (dal.findByID(r1.id.value()).has_value());
    EXPECT_FALSE(dal.findByID(r2.id.value()).has_value());
}