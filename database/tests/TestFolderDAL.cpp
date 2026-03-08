#include <fstream>
#include <gtest/gtest.h>
#include <sqlite3.h>
#include <sstream>

#include "../DAL/FolderDAL.h"
#include "../DAL/MessageDAL.h"
#include "../Entity/Folder.h"

class FolderDALTest : public ::testing::Test
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
					 "INSERT INTO users (id, username, password_hash) VALUES (1, 'alice', 'hash');"
					 "INSERT INTO users (id, username, password_hash) VALUES (2, 'bob',   'hash');",
					 nullptr, nullptr, nullptr);

		m_dal = new FolderDAL(m_db);
	}

	void TearDown() override
	{
		delete m_dal;
		sqlite3_close(m_db);
	}

	Folder make(int64_t user_id = 1, const std::string& name = "Inbox")
	{
		Folder f;
		f.user_id = user_id;
		f.name = name;
		return f;
	}

	sqlite3* m_db = nullptr;
	FolderDAL* m_dal = nullptr;
};

TEST_F(FolderDALTest, Insert_SetsID)
{
	auto f = make();
	EXPECT_TRUE(m_dal->insert(f));
	EXPECT_TRUE(f.id.has_value());
}

TEST_F(FolderDALTest, Insert_PersistsFields)
{
	auto f = make(1, "Work");
	m_dal->insert(f);

	auto found = m_dal->findByID(f.id.value());
	ASSERT_TRUE(found.has_value());
	EXPECT_EQ(found->user_id, 1);
	EXPECT_EQ(found->name, "Work");
}

TEST_F(FolderDALTest, Insert_RejectsUnknownUser)
{
	auto f = make(99999);
	EXPECT_FALSE(m_dal->insert(f));
}

TEST_F(FolderDALTest, Insert_AllowsSameNameForDifferentUsers)
{
	auto f1 = make(1, "Inbox");
	auto f2 = make(2, "Inbox");
	EXPECT_TRUE(m_dal->insert(f1));
	EXPECT_TRUE(m_dal->insert(f2));
}

TEST_F(FolderDALTest, FindByID_ReturnsNulloptIfMissing)
{
	EXPECT_FALSE(m_dal->findByID(99999).has_value());
}

TEST_F(FolderDALTest, FindByUser_ReturnsOnlyThatUser)
{
	auto f1 = make(1, "Inbox");
	auto f2 = make(1, "Sent");
	auto f3 = make(2, "Inbox");
	m_dal->insert(f1);
	m_dal->insert(f2);
	m_dal->insert(f3);

	auto r = m_dal->findByUser(1);
	EXPECT_EQ(r.size(), 2u);
	for (const auto& f : r)
		EXPECT_EQ(f.user_id, 1);
}

TEST_F(FolderDALTest, FindByUser_EmptyWhenNone)
{
	EXPECT_TRUE(m_dal->findByUser(1).empty());
}

TEST_F(FolderDALTest, FindByName_ReturnsCorrectFolder)
{
	auto f = make(1, "Work");
	m_dal->insert(f);
	auto result = m_dal->findByName(1, "Work");
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(result->name, "Work");
}

TEST_F(FolderDALTest, FindByName_DoesNotReturnOtherUsersFolder)
{
	auto f = make(2, "Inbox");
	m_dal->insert(f);
	EXPECT_FALSE(m_dal->findByName(1, "Inbox").has_value());
}

TEST_F(FolderDALTest, Update_PersistsNameChange)
{
	auto f = make(1, "Old");
	m_dal->insert(f);
	f.name = "New";
	EXPECT_TRUE(m_dal->update(f));
	EXPECT_EQ(m_dal->findByID(f.id.value())->name, "New");
}

TEST_F(FolderDALTest, Update_FailsWithNoID)
{
	auto f = make();
	EXPECT_FALSE(m_dal->update(f));
}

TEST_F(FolderDALTest, HardDelete_RemovesFolder)
{
	auto f = make();
	m_dal->insert(f);
	int64_t id = f.id.value();
	EXPECT_TRUE(m_dal->hardDelete(id));
	EXPECT_FALSE(m_dal->findByID(id).has_value());
}

TEST_F(FolderDALTest, HardDelete_OnlyRemovesTarget)
{
	auto f1 = make(1, "Keep");
	m_dal->insert(f1);
	auto f2 = make(1, "Delete");
	m_dal->insert(f2);
	m_dal->hardDelete(f2.id.value());

	EXPECT_TRUE(m_dal->findByID(f1.id.value()).has_value());
	EXPECT_FALSE(m_dal->findByID(f2.id.value()).has_value());
}

TEST_F(FolderDALTest, Cascade_DeletedWhenUserDeleted)
{
	auto f = make();
	m_dal->insert(f);
	int64_t id = f.id.value();
	sqlite3_exec(m_db, "DELETE FROM users WHERE id = 1;", nullptr, nullptr, nullptr);
	EXPECT_FALSE(m_dal->findByID(id).has_value());
}

TEST_F(FolderDALTest, Cascade_MessageFolderSetNullWhenFolderDeleted)
{
	auto f = make(1, "Temp");
	m_dal->insert(f);

	MessageDAL msgDal(m_db);
	Message msg;
	msg.user_id = 1;
	msg.folder_id = f.id.value();
	msg.receiver = "b@b.com";
	msg.subject = "T";
	msg.status = MessageStatus::Draft;
	msgDal.insert(msg);

	m_dal->hardDelete(f.id.value());
	EXPECT_FALSE(msgDal.findByID(msg.id.value())->folder_id.has_value());
}