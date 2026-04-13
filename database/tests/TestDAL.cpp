#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <string>
#include <unistd.h>

#include "DataBaseManager.h"
#include "DAL/RecipientDAL.h"
#include "DAL/UserDAL.h"
#include "DAL/MessageDAL.h"
#include "Entity/Folder.h"
#include "DAL/FolderDAL.h"
#include "schema.h"

namespace {

std::atomic<int> g_dal_counter{0};

std::string uniqueDbPath() {
    auto tmp = std::filesystem::temp_directory_path();
    return (tmp / ("dal_test_" + std::to_string(getpid()) + "_" + std::to_string(++g_dal_counter) + ".db")).string();
}

void removeDb(const std::string& path) {
    for (const char* suffix : {"", "-wal", "-shm"})
        std::filesystem::remove(path + suffix);
}

} 

class DALFixture : public ::testing::Test {
protected:
    std::string                      m_path;
    std::unique_ptr<DataBaseManager> m_mgr;

    void SetUp() override {
        m_path = uniqueDbPath();
        m_mgr  = std::make_unique<DataBaseManager>(m_path, initSchema(), nullptr, 1);
        ASSERT_TRUE(m_mgr->isConnected()) << "DB failed: " << m_path;
    }

    void TearDown() override {
        m_mgr.reset();
        removeDb(m_path);
    }

    sqlite3*        db()   { return m_mgr->getDB(); }
    ConnectionPool& pool() { return m_mgr->pool(); }


    int64_t insertUser(const std::string& username) {
        UserDAL dal(db(), pool());
        User u;
        u.username      = username;
        u.password_hash = "hash";
        EXPECT_TRUE(dal.insert(u));
        return u.id.value();
    }


    int64_t insertFolder(int64_t user_id, const std::string& name) {
        FolderDAL dal(db(), pool());
        Folder f;
        f.user_id = user_id;
        f.name    = name;
        EXPECT_TRUE(dal.insert(f));
        return f.id.value();
    }


    int64_t insertMessage(int64_t user_id, int64_t folder_id) {
        MessageDAL dal(db(), pool());
        Message m;
        m.user_id       = user_id;
        m.folder_id     = folder_id;
        m.uid           = 1;
        m.raw_file_path = "/tmp/test.eml";
        m.size_bytes    = 100;
        m.from_address  = "from@example.com";
        m.internal_date = "2024-01-01T00:00:00Z";
        EXPECT_TRUE(dal.insert(m));
        return m.id.value();
    }
};


class RecipientDALTest : public DALFixture {
protected:
    int64_t m_user_id{0};
    int64_t m_folder_id{0};
    int64_t m_message_id{0};

    void SetUp() override {
        DALFixture::SetUp();
        m_user_id    = insertUser("alice");
        m_folder_id  = insertFolder(m_user_id, "INBOX");
        m_message_id = insertMessage(m_user_id, m_folder_id);
    }
};

TEST_F(RecipientDALTest, InsertAndFindByID) {
    RecipientDAL dal(db(), pool());
    Recipient r;
    r.message_id = m_message_id;
    r.address    = "to@example.com";
    r.type       = RecipientType::To;

    ASSERT_TRUE(dal.insert(r));
    ASSERT_TRUE(r.id.has_value());

    auto found = dal.findByID(r.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->address, "to@example.com");
    EXPECT_EQ(found->type, RecipientType::To);
}

TEST_F(RecipientDALTest, UpdateChangesFields) {
    RecipientDAL dal(db(), pool());
    Recipient r;
    r.message_id = m_message_id;
    r.address    = "orig@example.com";
    r.type       = RecipientType::To;
    ASSERT_TRUE(dal.insert(r));

    r.address = "updated@example.com";
    r.type    = RecipientType::Cc;
    EXPECT_TRUE(dal.update(r));

    auto found = dal.findByID(r.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->address, "updated@example.com");
    EXPECT_EQ(found->type, RecipientType::Cc);
}

TEST_F(RecipientDALTest, UpdateWithNoIdFails) {
    RecipientDAL dal(db(), pool());
    Recipient r;
    r.message_id = m_message_id;
    r.address    = "no@id.com";
    r.type       = RecipientType::To;
    // r.id is not set

    EXPECT_FALSE(dal.update(r));
    EXPECT_FALSE(dal.getLastError().empty());
}

TEST_F(RecipientDALTest, HardDeleteRemovesRow) {
    RecipientDAL dal(db(), pool());
    Recipient r;
    r.message_id = m_message_id;
    r.address    = "del@example.com";
    r.type       = RecipientType::Bcc;
    ASSERT_TRUE(dal.insert(r));
    int64_t id = r.id.value();

    EXPECT_TRUE(dal.hardDelete(id));
    EXPECT_FALSE(dal.findByID(id).has_value());
}

TEST_F(RecipientDALTest, HardDeleteNonExistentSucceeds) {
    RecipientDAL dal(db(), pool());
    EXPECT_TRUE(dal.hardDelete(999999));
}

TEST_F(RecipientDALTest, FindByMessageReturnsAll) {
    RecipientDAL dal(db(), pool());

    for (auto addr : {"a@x.com", "b@x.com", "c@x.com"}) {
        Recipient r;
        r.message_id = m_message_id;
        r.address    = addr;
        r.type       = RecipientType::To;
        ASSERT_TRUE(dal.insert(r));
    }

    auto rows = dal.findByMessage(m_message_id);
    EXPECT_EQ(rows.size(), 3u);
}

TEST_F(RecipientDALTest, AllRecipientTypes) {
    RecipientDAL dal(db(), pool());

    for (auto type : {RecipientType::To, RecipientType::Cc, RecipientType::Bcc, RecipientType::ReplyTo}) {
        Recipient r;
        r.message_id = m_message_id;
        r.address    = "test@example.com";
        r.type       = type;
        ASSERT_TRUE(dal.insert(r));

        auto found = dal.findByID(r.id.value());
        ASSERT_TRUE(found.has_value());
        EXPECT_EQ(found->type, type);
    }
}

class UserDALTest : public DALFixture {};

TEST_F(UserDALTest, InsertAndFindByID) {
    UserDAL dal(db(), pool());
    User u;
    u.username      = "bob";
    u.password_hash = "hash123";

    ASSERT_TRUE(dal.insert(u));
    ASSERT_TRUE(u.id.has_value());

    auto found = dal.findByID(u.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->username, "bob");
}

TEST_F(UserDALTest, UpdateWithNoIdFails) {
    UserDAL dal(db(), pool());
    User u;
    u.username      = "ghost";
    u.password_hash = "hash";
    // no id

    EXPECT_FALSE(dal.update(u));
    EXPECT_FALSE(dal.getLastError().empty());
}

TEST_F(UserDALTest, UpdateChangesFields) {
    UserDAL dal(db(), pool());
    User u;
    u.username      = "charlie";
    u.password_hash = "oldhash";
    ASSERT_TRUE(dal.insert(u));

    u.username      = "charlie_updated";
    u.password_hash = "newhash";
    u.first_name    = "Charlie";
    EXPECT_TRUE(dal.update(u));

    auto found = dal.findByID(u.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->username, "charlie_updated");
    EXPECT_EQ(found->first_name, "Charlie");
}

TEST_F(UserDALTest, GetAvatarNoAvatarReturnsNullopt) {
    UserDAL dal(db(), pool());
    User u;
    u.username      = "noavatar";
    u.password_hash = "hash";
    ASSERT_TRUE(dal.insert(u));

    auto avatar = dal.getAvatar(u.id.value());
    EXPECT_FALSE(avatar.has_value());
}

TEST_F(UserDALTest, GetAvatarNonExistentReturnsNullopt) {
    UserDAL dal(db(), pool());
    auto avatar = dal.getAvatar(999999);
    EXPECT_FALSE(avatar.has_value());
}

TEST_F(UserDALTest, GetAvatarAfterUpdateAvatar) {
    UserDAL dal(db(), pool());
    User u;
    u.username      = "withavatar";
    u.password_hash = "hash";
    ASSERT_TRUE(dal.insert(u));

    EXPECT_TRUE(dal.updateAvatar(u.id.value(), "base64data=="));

    auto avatar = dal.getAvatar(u.id.value());
    ASSERT_TRUE(avatar.has_value());
    EXPECT_EQ(*avatar, "base64data==");
}

TEST_F(UserDALTest, GetAvatarClearedReturnsNullopt) {
    UserDAL dal(db(), pool());
    User u;
    u.username      = "clearavatar";
    u.password_hash = "hash";
    ASSERT_TRUE(dal.insert(u));

    EXPECT_TRUE(dal.updateAvatar(u.id.value(), "somedata"));
    EXPECT_TRUE(dal.updateAvatar(u.id.value(), std::nullopt));

    auto avatar = dal.getAvatar(u.id.value());
    EXPECT_FALSE(avatar.has_value());
}

TEST_F(UserDALTest, HardDeleteRemovesUser) {
    UserDAL dal(db(), pool());
    User u;
    u.username      = "todelete";
    u.password_hash = "hash";
    ASSERT_TRUE(dal.insert(u));
    int64_t id = u.id.value();

    EXPECT_TRUE(dal.hardDelete(id));
    EXPECT_FALSE(dal.findByID(id).has_value());
}

TEST_F(UserDALTest, UpdateProfileNullOptValues) {
    UserDAL dal(db(), pool());
    User u;
    u.username      = "proftest";
    u.password_hash = "hash";
    ASSERT_TRUE(dal.insert(u));

    EXPECT_TRUE(dal.updateProfile(u.id.value(), std::nullopt, std::nullopt, std::nullopt));

    auto found = dal.findByID(u.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_FALSE(found->first_name.has_value());
    EXPECT_FALSE(found->last_name.has_value());
    EXPECT_FALSE(found->birthdate.has_value());
}

TEST_F(UserDALTest, UpdateProfileWithValues) {
    UserDAL dal(db(), pool());
    User u;
    u.username      = "proftest2";
    u.password_hash = "hash";
    ASSERT_TRUE(dal.insert(u));

    EXPECT_TRUE(dal.updateProfile(u.id.value(), "John", "Doe", "1990-01-01"));

    auto found = dal.findByID(u.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->first_name, "John");
    EXPECT_EQ(found->last_name, "Doe");
    EXPECT_EQ(found->birthdate, "1990-01-01");
}

class MessageDALTest : public DALFixture {
protected:
    int64_t m_user_id{0};
    int64_t m_folder_id{0};
    int64_t m_folder2_id{0};

    void SetUp() override {
        DALFixture::SetUp();
        m_user_id   = insertUser("msguser");
        m_folder_id = insertFolder(m_user_id, "INBOX");
        m_folder2_id = insertFolder(m_user_id, "Sent");
    }

    Message buildMsg(int64_t uid = 1) {
        Message m;
        m.user_id       = m_user_id;
        m.folder_id     = m_folder_id;
        m.uid           = uid;
        m.raw_file_path = "/tmp/m.eml";
        m.size_bytes    = 200;
        m.from_address  = "from@test.com";
        m.internal_date = "2024-06-01T00:00:00Z";
        m.is_recent     = true;
        return m;
    }
};

TEST_F(MessageDALTest, UpdateFlagsAllTrue) {
    MessageDAL dal(db(), pool());
    Message m = buildMsg(1);
    ASSERT_TRUE(dal.insert(m));

    EXPECT_TRUE(dal.updateFlags(m.id.value(), true, true, true, true, true, true));

    auto found = dal.findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_TRUE(found->is_seen);
    EXPECT_TRUE(found->is_deleted);
    EXPECT_TRUE(found->is_draft);
    EXPECT_TRUE(found->is_answered);
    EXPECT_TRUE(found->is_flagged);
    EXPECT_TRUE(found->is_recent);
}

TEST_F(MessageDALTest, UpdateFlagsAllFalse) {
    MessageDAL dal(db(), pool());
    Message m = buildMsg(2);
    m.is_seen = m.is_flagged = true;
    ASSERT_TRUE(dal.insert(m));

    EXPECT_TRUE(dal.updateFlags(m.id.value(), false, false, false, false, false, false));

    auto found = dal.findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_FALSE(found->is_seen);
    EXPECT_FALSE(found->is_flagged);
}

TEST_F(MessageDALTest, MoveToFolder) {
    MessageDAL dal(db(), pool());
    Message m = buildMsg(3);
    ASSERT_TRUE(dal.insert(m));

    EXPECT_TRUE(dal.moveToFolder(m.id.value(), m_folder2_id, 10));

    auto found = dal.findByID(m.id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->folder_id, m_folder2_id);
    EXPECT_EQ(found->uid, 10);
}

TEST_F(MessageDALTest, ClearRecentByFolder) {
    MessageDAL dal(db(), pool());

    Message m1 = buildMsg(4); m1.is_recent = true;
    Message m2 = buildMsg(5); m2.is_recent = true;
    ASSERT_TRUE(dal.insert(m1));
    ASSERT_TRUE(dal.insert(m2));

    EXPECT_TRUE(dal.clearRecentByFolder(m_folder_id));

    auto f1 = dal.findByID(m1.id.value());
    auto f2 = dal.findByID(m2.id.value());
    ASSERT_TRUE(f1.has_value());
    ASSERT_TRUE(f2.has_value());
    EXPECT_FALSE(f1->is_recent);
    EXPECT_FALSE(f2->is_recent);
}

TEST_F(MessageDALTest, ClearRecentOnlyAffectsTargetFolder) {
    MessageDAL dal(db(), pool());

    Message m1 = buildMsg(6); m1.is_recent = true;
    Message m2 = buildMsg(7); m2.folder_id = m_folder2_id; m2.is_recent = true;
    ASSERT_TRUE(dal.insert(m1));
    ASSERT_TRUE(dal.insert(m2));

    EXPECT_TRUE(dal.clearRecentByFolder(m_folder_id));

    auto f1 = dal.findByID(m1.id.value());
    auto f2 = dal.findByID(m2.id.value());
    ASSERT_TRUE(f1.has_value());
    ASSERT_TRUE(f2.has_value());
    EXPECT_FALSE(f1->is_recent);
    EXPECT_TRUE(f2->is_recent);
}

TEST_F(MessageDALTest, HardDeleteRemovesMessage) {
    MessageDAL dal(db(), pool());
    Message m = buildMsg(8);
    ASSERT_TRUE(dal.insert(m));
    int64_t id = m.id.value();

    EXPECT_TRUE(dal.hardDelete(id));
    EXPECT_FALSE(dal.findByID(id).has_value());
}

TEST_F(MessageDALTest, UpdateWithNoIdFails) {
    MessageDAL dal(db(), pool());
    Message m = buildMsg(9);
    // no id set
    EXPECT_FALSE(dal.update(m));
    EXPECT_FALSE(dal.getLastError().empty());
}
