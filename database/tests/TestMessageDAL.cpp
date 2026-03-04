#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include <sqlite3.h>

#include "../DAL/MessageDAL.h"
#include "../Entity/Message.h"

class MessageDALTest : public ::testing::Test
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
            "INSERT INTO users   (id, username, password_hash) VALUES (2, 'bob',   'hash');"
            "INSERT INTO folders (id, user_id, name) VALUES (1, 1, 'Inbox');"
            "INSERT INTO folders (id, user_id, name) VALUES (2, 1, 'Archive');",
            nullptr, nullptr, nullptr);

        m_dal = new MessageDAL(m_db);
    }

    void TearDown() override
    {
        delete m_dal;
        sqlite3_close(m_db);
    }

    Message make(const std::string& subject = "Subject",
                 const std::string& receiver = "bob@example.com",
                 MessageStatus status = MessageStatus::Draft)
    {
        Message msg;
        msg.user_id   = 1;
        msg.folder_id = 1;
        msg.subject   = subject;
        msg.receiver  = receiver;
        msg.body      = "Body.";
        msg.status    = status;
        return msg;
    }

    sqlite3*    m_db  = nullptr;
    MessageDAL* m_dal = nullptr;
};

TEST_F(MessageDALTest, Insert_SetsID)
{
    auto msg = make();
    EXPECT_TRUE(m_dal->insert(msg));
    EXPECT_TRUE(msg.id.has_value());
}

TEST_F(MessageDALTest, Insert_PersistsFields)
{
    auto msg = make("Hello", "alice@example.com", MessageStatus::Sent);
    msg.is_seen = msg.is_starred = msg.is_important = true;
    m_dal->insert(msg);

    auto f = m_dal->findByID(msg.id.value());
    ASSERT_TRUE(f.has_value());
    EXPECT_EQ(f->subject,  "Hello");
    EXPECT_EQ(f->receiver, "alice@example.com");
    EXPECT_EQ(f->status,   MessageStatus::Sent);
    EXPECT_TRUE(f->is_seen);
    EXPECT_TRUE(f->is_starred);
    EXPECT_TRUE(f->is_important);
}

TEST_F(MessageDALTest, Insert_NullFolderAllowed)
{
    auto msg = make(); msg.folder_id = std::nullopt;
    EXPECT_TRUE(m_dal->insert(msg));
}

TEST_F(MessageDALTest, Insert_RejectsUnknownUser)
{
    auto msg = make(); msg.user_id = 99999;
    EXPECT_FALSE(m_dal->insert(msg));
}

TEST_F(MessageDALTest, Insert_RejectsUnknownFolder)
{
    auto msg = make(); msg.folder_id = 99999;
    EXPECT_FALSE(m_dal->insert(msg));
}

TEST_F(MessageDALTest, FindByID_ReturnsNulloptIfMissing)
{
    EXPECT_FALSE(m_dal->findByID(99999).has_value());
}

TEST_F(MessageDALTest, FindByUser_ReturnsOnlyThatUser)
{
    m_dal->insert(make("M1")); m_dal->insert(make("M2"));
    Message m3 = make("M3"); m3.user_id = 2; m3.folder_id = std::nullopt;
    m_dal->insert(m3);

    auto r = m_dal->findByUser(1);
    EXPECT_EQ(r.size(), 2u);
    for (const auto& m : r) EXPECT_EQ(m.user_id, 1);
}

TEST_F(MessageDALTest, FindByFolder_ReturnsOnlyThatFolder)
{
    auto m1 = make(); m1.folder_id = 1; m_dal->insert(m1);
    auto m2 = make(); m2.folder_id = 2; m_dal->insert(m2);
    auto m3 = make(); m3.folder_id = 2; m_dal->insert(m3);

    EXPECT_EQ(m_dal->findByFolder(2).size(), 2u);
}

TEST_F(MessageDALTest, FindByStatus_ReturnsOnlyMatchingStatus)
{
    m_dal->insert(make("D", "a@b.com", MessageStatus::Draft));
    m_dal->insert(make("S", "a@b.com", MessageStatus::Sent));
    m_dal->insert(make("S2","a@b.com", MessageStatus::Sent));

    auto r = m_dal->findByStatus(1, MessageStatus::Sent);
    EXPECT_EQ(r.size(), 2u);
}

TEST_F(MessageDALTest, FindUnseen_ReturnsOnlyUnseen)
{
    auto m1 = make(); m1.is_seen = false; m_dal->insert(m1);
    auto m2 = make(); m2.is_seen = true;  m_dal->insert(m2);

    EXPECT_EQ(m_dal->findUnseen(1).size(), 1u);
}

TEST_F(MessageDALTest, FindStarred_ReturnsOnlyStarred)
{
    auto m1 = make(); m1.is_starred = true;  m_dal->insert(m1);
    auto m2 = make(); m2.is_starred = false; m_dal->insert(m2);

    EXPECT_EQ(m_dal->findStarred(1).size(), 1u);
}

TEST_F(MessageDALTest, FindImportant_ReturnsOnlyImportant)
{
    auto m1 = make(); m1.is_important = true;  m_dal->insert(m1);
    auto m2 = make(); m2.is_important = false; m_dal->insert(m2);

    EXPECT_EQ(m_dal->findImportant(1).size(), 1u);
}

TEST_F(MessageDALTest, Search_FindsBySubject)
{
    auto msg = make("Quarterly Report"); m_dal->insert(msg);
    EXPECT_EQ(m_dal->search(1, "Quarterly").size(), 1u);
}

TEST_F(MessageDALTest, Search_FindsByBody)
{
    auto msg = make(); msg.body = "invoice attached"; m_dal->insert(msg);
    EXPECT_EQ(m_dal->search(1, "invoice").size(), 1u);
}

TEST_F(MessageDALTest, Search_CaseInsensitive)
{
    auto msg = make("URGENT"); m_dal->insert(msg);
    EXPECT_EQ(m_dal->search(1, "urgent").size(), 1u);
}

TEST_F(MessageDALTest, Search_NoMatchReturnsEmpty)
{
    auto msg = make(); m_dal->insert(msg);
    EXPECT_TRUE(m_dal->search(1, "zzz").empty());
}

TEST_F(MessageDALTest, Search_DoesNotReturnOtherUsersMessages)
{
    auto m1 = make("Keyword"); m1.folder_id = std::nullopt; m_dal->insert(m1);
    Message m2 = make("Keyword"); m2.user_id = 2; m2.folder_id = std::nullopt; m_dal->insert(m2);

    EXPECT_EQ(m_dal->search(1, "Keyword").size(), 1u);
}

TEST_F(MessageDALTest, Update_PersistsChanges)
{
    auto msg = make(); m_dal->insert(msg);
    msg.subject = "Updated"; msg.body = "Updated body.";
    EXPECT_TRUE(m_dal->update(msg));
    EXPECT_EQ(m_dal->findByID(msg.id.value())->subject, "Updated");
}

TEST_F(MessageDALTest, Update_FailsWithNoID)
{
    auto msg = make();
    EXPECT_FALSE(m_dal->update(msg));
}

TEST_F(MessageDALTest, UpdateStatus_ChangesStatus)
{
    auto msg = make(); m_dal->insert(msg);
    EXPECT_TRUE(m_dal->updateStatus(msg.id.value(), MessageStatus::Sent));
    EXPECT_EQ(m_dal->findByID(msg.id.value())->status, MessageStatus::Sent);
}

TEST_F(MessageDALTest, UpdateFlags_PersistsAllFlags)
{
    auto msg = make(); m_dal->insert(msg);
    EXPECT_TRUE(m_dal->updateFlags(msg.id.value(), true, true, true));

    auto f = m_dal->findByID(msg.id.value());
    EXPECT_TRUE(f->is_seen);
    EXPECT_TRUE(f->is_starred);
    EXPECT_TRUE(f->is_important);
}

TEST_F(MessageDALTest, UpdateSeen_Toggles)
{
    auto msg = make(); m_dal->insert(msg);
    m_dal->updateSeen(msg.id.value(), true);
    EXPECT_TRUE(m_dal->findByID(msg.id.value())->is_seen);
    m_dal->updateSeen(msg.id.value(), false);
    EXPECT_FALSE(m_dal->findByID(msg.id.value())->is_seen);
}

TEST_F(MessageDALTest, MoveToFolder_Updates)
{
    auto msg = make(); m_dal->insert(msg);
    EXPECT_TRUE(m_dal->moveToFolder(msg.id.value(), 2));
    EXPECT_EQ(m_dal->findByID(msg.id.value())->folder_id, 2);
}

TEST_F(MessageDALTest, MoveToFolder_AcceptsNullopt)
{
    auto msg = make(); m_dal->insert(msg);
    EXPECT_TRUE(m_dal->moveToFolder(msg.id.value(), std::nullopt));
    EXPECT_FALSE(m_dal->findByID(msg.id.value())->folder_id.has_value());
}

TEST_F(MessageDALTest, MoveToFolder_RejectsUnknown)
{
    auto msg = make(); m_dal->insert(msg);
    EXPECT_FALSE(m_dal->moveToFolder(msg.id.value(), 99999));
}

TEST_F(MessageDALTest, SoftDelete_SetsDeleted)
{
    auto msg = make(); m_dal->insert(msg);
    m_dal->softDelete(msg.id.value());
    EXPECT_EQ(m_dal->findByID(msg.id.value())->status, MessageStatus::Deleted);
}

TEST_F(MessageDALTest, SoftDelete_RowStillExists)
{
    auto msg = make(); m_dal->insert(msg);
    m_dal->softDelete(msg.id.value());
    EXPECT_TRUE(m_dal->findByID(msg.id.value()).has_value());
}

TEST_F(MessageDALTest, HardDelete_RemovesRow)
{
    auto msg = make(); m_dal->insert(msg);
    int64_t id = msg.id.value();
    EXPECT_TRUE(m_dal->hardDelete(id));
    EXPECT_FALSE(m_dal->findByID(id).has_value());
}

TEST_F(MessageDALTest, Isolation_StatusQueryDoesNotLeakAcrossUsers)
{
    auto m1 = make("U1", "a@b.com", MessageStatus::Sent); m1.folder_id = std::nullopt; m_dal->insert(m1);
    Message m2 = make("U2", "a@b.com", MessageStatus::Sent); m2.user_id = 2; m2.folder_id = std::nullopt; m_dal->insert(m2);

    EXPECT_EQ(m_dal->findByStatus(1, MessageStatus::Sent).size(), 1u);
}