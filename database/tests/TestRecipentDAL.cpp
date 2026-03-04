#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include <sqlite3.h>

#include "../DAL/RecipientDAL.h"
#include "../DAL/MessageDAL.h"
#include "../Entity/Recipient.h"

class RecipientDALTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        sqlite3_open(":memory:", &m_db);

        std::ifstream file(SCHEMA_PATH);
        std::stringstream ss;
        ss << file.rdbuf();
        sqlite3_exec(m_db, ss.str().c_str(), nullptr, nullptr, nullptr);

        sqlite3_exec(m_db,
            "INSERT INTO users   (id, username, password_hash) VALUES (1, 'alice', 'hash');"
            "INSERT INTO folders (id, user_id, name) VALUES (1, 1, 'Inbox');",
            nullptr, nullptr, nullptr);

        MessageDAL msgDal(m_db);
        Message m1; m1.user_id = 1; m1.folder_id = 1; m1.receiver = "b@b.com"; m1.subject = "M1"; m1.status = MessageStatus::Draft;
        Message m2; m2.user_id = 1; m2.folder_id = 1; m2.receiver = "c@c.com"; m2.subject = "M2"; m2.status = MessageStatus::Sent;
        msgDal.insert(m1); msgDal.insert(m2);
        m_msg1 = m1.id.value();
        m_msg2 = m2.id.value();

        m_dal = new RecipientDAL(m_db);
    }

    void TearDown() override
    {
        delete m_dal;
        sqlite3_close(m_db);
    }

    Recipient make(int64_t msg_id = 0, const std::string& address = "alice@example.com")
    {
        Recipient r;
        r.message_id = msg_id == 0 ? m_msg1 : msg_id;
        r.address    = address;
        return r;
    }

    sqlite3*      m_db  = nullptr;
    RecipientDAL* m_dal = nullptr;
    int64_t       m_msg1 = 0;
    int64_t       m_msg2 = 0;
};

TEST_F(RecipientDALTest, Insert_SetsID)
{
    auto r = make();
    EXPECT_TRUE(m_dal->insert(r));
    EXPECT_TRUE(r.id.has_value());
}

TEST_F(RecipientDALTest, Insert_PersistsFields)
{
    auto r = make(m_msg1, "specific@example.com");
    m_dal->insert(r);

    auto f = m_dal->findByID(r.id.value());
    ASSERT_TRUE(f.has_value());
    EXPECT_EQ(f->address,    "specific@example.com");
    EXPECT_EQ(f->message_id, m_msg1);
}

TEST_F(RecipientDALTest, Insert_RejectsUnknownMessage)
{
    auto r = make(99999);
    EXPECT_FALSE(m_dal->insert(r));
}

TEST_F(RecipientDALTest, Insert_AllowsMultiplePerMessage)
{
    EXPECT_TRUE(m_dal->insert(make(m_msg1, "a@example.com")));
    EXPECT_TRUE(m_dal->insert(make(m_msg1, "b@example.com")));
    EXPECT_TRUE(m_dal->insert(make(m_msg1, "c@example.com")));
}

TEST_F(RecipientDALTest, Insert_AllowsSameAddressOnDifferentMessages)
{
    EXPECT_TRUE(m_dal->insert(make(m_msg1, "shared@example.com")));
    EXPECT_TRUE(m_dal->insert(make(m_msg2, "shared@example.com")));
}

TEST_F(RecipientDALTest, FindByID_ReturnsNulloptIfMissing)
{
    EXPECT_FALSE(m_dal->findByID(99999).has_value());
}

TEST_F(RecipientDALTest, FindByMessage_ReturnsAll)
{
    m_dal->insert(make(m_msg1, "a@example.com"));
    m_dal->insert(make(m_msg1, "b@example.com"));
    m_dal->insert(make(m_msg2, "c@example.com"));

    EXPECT_EQ(m_dal->findByMessage(m_msg1).size(), 2u);
}

TEST_F(RecipientDALTest, FindByMessage_EmptyWhenNone)
{
    EXPECT_TRUE(m_dal->findByMessage(m_msg1).empty());
}

TEST_F(RecipientDALTest, Update_PersistsAddressChange)
{
    auto r = make(m_msg1, "old@example.com");
    m_dal->insert(r);
    r.address = "new@example.com";
    EXPECT_TRUE(m_dal->update(r));
    EXPECT_EQ(m_dal->findByID(r.id.value())->address, "new@example.com");
}

TEST_F(RecipientDALTest, Update_FailsWithNoID)
{
    auto r = make();
    EXPECT_FALSE(m_dal->update(r));
}

TEST_F(RecipientDALTest, HardDelete_RemovesRow)
{
    auto r = make(); m_dal->insert(r);
    int64_t id = r.id.value();
    EXPECT_TRUE(m_dal->hardDelete(id));
    EXPECT_FALSE(m_dal->findByID(id).has_value());
}

TEST_F(RecipientDALTest, HardDelete_OnlyRemovesTarget)
{
    auto r1 = make(m_msg1, "keep@example.com");
    auto r2 = make(m_msg1, "del@example.com");
    m_dal->insert(r1); m_dal->insert(r2);
    m_dal->hardDelete(r2.id.value());

    EXPECT_TRUE(m_dal->findByID(r1.id.value()).has_value());
    EXPECT_FALSE(m_dal->findByID(r2.id.value()).has_value());
}

TEST_F(RecipientDALTest, Cascade_DeletedWhenMessageDeleted)
{
    m_dal->insert(make(m_msg1, "a@example.com"));
    m_dal->insert(make(m_msg1, "b@example.com"));

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(m_db, "DELETE FROM messages WHERE id = ?;", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, m_msg1);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    EXPECT_TRUE(m_dal->findByMessage(m_msg1).empty());
}