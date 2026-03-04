#include <fstream>
#include <gtest/gtest.h>
#include <sqlite3.h>
#include <sstream>

#include "../DAL/AttachmentDAL.h"
#include "../DAL/MessageDAL.h"
#include "../Entity/Attachment.h"

class AttachmentDALTest : public ::testing::Test
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
		Message m1;
		m1.user_id = 1;
		m1.folder_id = 1;
		m1.receiver = "b@b.com";
		m1.subject = "M1";
		m1.status = MessageStatus::Draft;
		Message m2;
		m2.user_id = 1;
		m2.folder_id = 1;
		m2.receiver = "c@c.com";
		m2.subject = "M2";
		m2.status = MessageStatus::Sent;
		msgDal.insert(m1);
		msgDal.insert(m2);
		m_msg1 = m1.id.value();
		m_msg2 = m2.id.value();

		m_dal = new AttachmentDAL(m_db);
	}

	void TearDown() override
	{
		delete m_dal;
		sqlite3_close(m_db);
	}

	Attachment make(int64_t msg_id = 0, const std::string& name = "file.pdf",
					const std::string& path = "/files/file.pdf", std::optional<std::string> mime = "application/pdf",
					std::optional<int64_t> size = 1024)
	{
		Attachment a;
		a.message_id = msg_id == 0 ? m_msg1 : msg_id;
		a.file_name = name;
		a.storage_path = path;
		a.mime_type = mime;
		a.file_size = size;
		return a;
	}

	sqlite3* m_db = nullptr;
	AttachmentDAL* m_dal = nullptr;
	int64_t m_msg1 = 0;
	int64_t m_msg2 = 0;
};

TEST_F(AttachmentDALTest, Insert_SetsID)
{
	auto a = make();
	EXPECT_TRUE(m_dal->insert(a));
	EXPECT_TRUE(a.id.has_value());
}

TEST_F(AttachmentDALTest, Insert_PersistsFields)
{
	auto a = make(m_msg1, "report.pdf", "/report.pdf", "application/pdf", 204800);
	m_dal->insert(a);

	auto f = m_dal->findByID(a.id.value());
	ASSERT_TRUE(f.has_value());
	EXPECT_EQ(f->file_name, "report.pdf");
	EXPECT_EQ(f->storage_path, "/report.pdf");
	EXPECT_EQ(f->mime_type, "application/pdf");
	EXPECT_EQ(f->file_size, 204800);
}

TEST_F(AttachmentDALTest, Insert_NullMimeAndSizeAllowed)
{
	auto a = make(m_msg1, "f.bin", "/f.bin", std::nullopt, std::nullopt);
	m_dal->insert(a);

	auto f = m_dal->findByID(a.id.value());
	ASSERT_TRUE(f.has_value());
	EXPECT_FALSE(f->mime_type.has_value());
	EXPECT_FALSE(f->file_size.has_value());
}

TEST_F(AttachmentDALTest, Insert_RejectsUnknownMessage)
{
	auto a = make(99999);
	EXPECT_FALSE(m_dal->insert(a));
}

TEST_F(AttachmentDALTest, FindByID_ReturnsNulloptIfMissing)
{
	EXPECT_FALSE(m_dal->findByID(99999).has_value());
}

TEST_F(AttachmentDALTest, FindByMessage_ReturnsAll)
{
	auto a1 = make(m_msg1, "a.pdf", "/a.pdf");
	auto a2 = make(m_msg1, "b.pdf", "/b.pdf");
	auto a3 = make(m_msg2, "c.pdf", "/c.pdf");
	m_dal->insert(a1);
	m_dal->insert(a2);
	m_dal->insert(a3);

	EXPECT_EQ(m_dal->findByMessage(m_msg1).size(), 2u);
}

TEST_F(AttachmentDALTest, FindByMessage_EmptyWhenNone)
{
	EXPECT_TRUE(m_dal->findByMessage(m_msg1).empty());
}

TEST_F(AttachmentDALTest, Update_PersistsChanges)
{
	auto a = make();
	m_dal->insert(a);
	a.file_name = "renamed.pdf";
	a.file_size = 9999;
	EXPECT_TRUE(m_dal->update(a));

	auto f = m_dal->findByID(a.id.value());
	EXPECT_EQ(f->file_name, "renamed.pdf");
	EXPECT_EQ(f->file_size, 9999);
}

TEST_F(AttachmentDALTest, Update_CanSetNullableToNull)
{
	auto a = make();
	m_dal->insert(a);
	a.mime_type = std::nullopt;
	a.file_size = std::nullopt;
	m_dal->update(a);

	auto f = m_dal->findByID(a.id.value());
	EXPECT_FALSE(f->mime_type.has_value());
	EXPECT_FALSE(f->file_size.has_value());
}

TEST_F(AttachmentDALTest, Update_FailsWithNoID)
{
	auto a = make();
	EXPECT_FALSE(m_dal->update(a));
}

TEST_F(AttachmentDALTest, HardDelete_RemovesRow)
{
	auto a = make();
	m_dal->insert(a);
	int64_t id = a.id.value();
	EXPECT_TRUE(m_dal->hardDelete(id));
	EXPECT_FALSE(m_dal->findByID(id).has_value());
}

TEST_F(AttachmentDALTest, HardDelete_OnlyRemovesTarget)
{
	auto a1 = make(m_msg1, "keep.pdf", "/keep.pdf");
	auto a2 = make(m_msg1, "del.pdf", "/del.pdf");
	m_dal->insert(a1);
	m_dal->insert(a2);
	m_dal->hardDelete(a2.id.value());

	EXPECT_TRUE(m_dal->findByID(a1.id.value()).has_value());
	EXPECT_FALSE(m_dal->findByID(a2.id.value()).has_value());
}

TEST_F(AttachmentDALTest, Cascade_DeletedWhenMessageDeleted)
{
	auto a = make();
	m_dal->insert(a);
	int64_t id = a.id.value();

	sqlite3_stmt* stmt = nullptr;
	sqlite3_prepare_v2(m_db, "DELETE FROM messages WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_msg1);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	EXPECT_FALSE(m_dal->findByID(id).has_value());
}