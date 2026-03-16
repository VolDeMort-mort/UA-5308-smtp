#include "TestFixture.h"
#include "../DAL/UserDAL.h"
#include "../DAL/FolderDAL.h"
#include "../DAL/MessageDAL.h"
#include "../DAL/RecipientDAL.h"

class RecipientDALTest : public DBFixture
{
protected:
    UserDAL*      userDal      = nullptr;
    FolderDAL*    folderDal    = nullptr;
    MessageDAL*   messageDal   = nullptr;
    RecipientDAL* recipientDal = nullptr;

    int64_t userId    = 0;
    int64_t folderId  = 0;
    int64_t messageId = 0;

    void SetUp() override
    {
        DBFixture::SetUp();
        userDal      = new UserDAL(db);
        folderDal    = new FolderDAL(db);
        messageDal   = new MessageDAL(db);
        recipientDal = new RecipientDAL(db);

        User u; u.username = "alice"; u.password_hash = "h";
        ASSERT_TRUE(userDal->insert(u));
        userId = u.id.value();

        Folder f; f.user_id = userId; f.name = "INBOX";
        f.next_uid = 1; f.is_subscribed = true;
        ASSERT_TRUE(folderDal->insert(f));
        folderId = f.id.value();

        Message m;
        m.user_id = userId; m.folder_id = folderId; m.uid = 1;
        m.raw_file_path = "/msg1.eml"; m.size_bytes = 100;
        m.from_address  = "sender@x.com"; m.internal_date = "2024-01-01";
        m.is_seen = m.is_deleted = m.is_draft =
        m.is_answered = m.is_flagged = false; m.is_recent = true;
        ASSERT_TRUE(messageDal->insert(m));
        messageId = m.id.value();
    }

    void TearDown() override
    {
        delete recipientDal;
        delete messageDal;
        delete folderDal;
        delete userDal;
        DBFixture::TearDown();
    }

    Recipient makeRecipient(const std::string& address,
                            const std::string& type = "To")
    {
        Recipient r;
        r.message_id = messageId;
        r.address    = address;
        r.type       = Recipient::typeFromString(type);
        EXPECT_TRUE(recipientDal->insert(r));
        EXPECT_TRUE(r.id.has_value());
        return r;
    }
};

// ── insert ────────────────────────────────────────────────────────────────────

TEST_F(RecipientDALTest, InsertSetsId)
{
    Recipient r = makeRecipient("alice@example.com");
    EXPECT_GT(r.id.value(), 0);
}

TEST_F(RecipientDALTest, InsertPreservesFields)
{
    Recipient r = makeRecipient("alice@example.com", "To");
    auto found  = recipientDal->findByID(r.id.value());

    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->address,    "alice@example.com");
    EXPECT_EQ(found->message_id, messageId);
    EXPECT_EQ(Recipient::typeToString(found->type), "To");
}

TEST_F(RecipientDALTest, InsertMultipleTypesAllowed)
{
    makeRecipient("to@example.com",  "To");
    makeRecipient("cc@example.com",  "Cc");
    makeRecipient("bcc@example.com", "Bcc");

    EXPECT_EQ(recipientDal->findByMessage(messageId, 100).size(), 3u);
}

// ── findByID ──────────────────────────────────────────────────────────────────

TEST_F(RecipientDALTest, FindByIDReturnsRecipient)
{
    Recipient r = makeRecipient("alice@example.com");
    auto found  = recipientDal->findByID(r.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->address, "alice@example.com");
}

TEST_F(RecipientDALTest, FindByIDUnknownReturnsNullopt)
{
    EXPECT_FALSE(recipientDal->findByID(9999).has_value());
}

// ── findByMessage ─────────────────────────────────────────────────────────────

TEST_F(RecipientDALTest, FindByMessageReturnsAllRecipients)
{
    makeRecipient("a@x.com");
    makeRecipient("b@x.com");
    EXPECT_EQ(recipientDal->findByMessage(messageId, 100).size(), 2u);
}

TEST_F(RecipientDALTest, FindByMessageEmptyReturnsEmpty)
{
    EXPECT_TRUE(recipientDal->findByMessage(messageId, 100).empty());
}

TEST_F(RecipientDALTest, FindByMessageLimitWorks)
{
    makeRecipient("a@x.com");
    makeRecipient("b@x.com");
    makeRecipient("c@x.com");

    EXPECT_EQ(recipientDal->findByMessage(messageId, 2, 0).size(), 2u);
    EXPECT_EQ(recipientDal->findByMessage(messageId, 2, 2).size(), 1u);
}

TEST_F(RecipientDALTest, FindByMessageDoesNotReturnOtherMessageRecipients)
{
    Message m2;
    m2.user_id = userId; m2.folder_id = folderId; m2.uid = 2;
    m2.raw_file_path = "/msg2.eml"; m2.size_bytes = 100;
    m2.from_address  = "s@x.com"; m2.internal_date = "2024-01-02";
    m2.is_seen = m2.is_deleted = m2.is_draft =
    m2.is_answered = m2.is_flagged = false; m2.is_recent = true;
    ASSERT_TRUE(messageDal->insert(m2));

    Recipient r2;
    r2.message_id = m2.id.value();
    r2.address    = "other@x.com";
    r2.type       = Recipient::typeFromString("To");
    ASSERT_TRUE(recipientDal->insert(r2));

    makeRecipient("mine@x.com");

    auto results = recipientDal->findByMessage(messageId, 100);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].address, "mine@x.com");
}

// ── update ────────────────────────────────────────────────────────────────────

TEST_F(RecipientDALTest, UpdateChangesAddress)
{
    Recipient r = makeRecipient("old@x.com");
    r.address   = "new@x.com";
    ASSERT_TRUE(recipientDal->update(r));

    auto found = recipientDal->findByID(r.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->address, "new@x.com");
}

TEST_F(RecipientDALTest, UpdateWithNoIdFails)
{
    Recipient r;
    r.message_id = messageId;
    r.address    = "x@x.com";
    r.type       = Recipient::typeFromString("To");
    EXPECT_FALSE(recipientDal->update(r));
}

TEST_F(RecipientDALTest, UpdateNonExistentFails)
{
    Recipient r;
    r.id         = 9999;
    r.message_id = messageId;
    r.address    = "x@x.com";
    r.type       = Recipient::typeFromString("To");
    EXPECT_FALSE(recipientDal->update(r));
}

// ── hardDelete ────────────────────────────────────────────────────────────────

TEST_F(RecipientDALTest, HardDeleteRemovesRecipient)
{
    Recipient r = makeRecipient("alice@x.com");
    ASSERT_TRUE(recipientDal->hardDelete(r.id.value()));
    EXPECT_FALSE(recipientDal->findByID(r.id.value()).has_value());
}

TEST_F(RecipientDALTest, HardDeleteNonExistentFails)
{
    EXPECT_FALSE(recipientDal->hardDelete(9999));
}

TEST_F(RecipientDALTest, HardDeleteDoesNotAffectOtherRecipients)
{
    Recipient a = makeRecipient("a@x.com");
    Recipient b = makeRecipient("b@x.com");
    ASSERT_TRUE(recipientDal->hardDelete(a.id.value()));
    EXPECT_TRUE(recipientDal->findByID(b.id.value()).has_value());
}