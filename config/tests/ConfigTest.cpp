#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include <unistd.h>

#include "Config.h"

using SmtpClient::Config;

class ConfigTest : public ::testing::Test
{
protected:
	void TearDown() override
	{
		std::remove(m_path.c_str());
	}

	void WriteJson(const std::string& json)
	{
		std::ofstream f(m_path);
		f << json;
	}

	const std::string m_path = "/tmp/test_app_config_" + std::to_string(getpid()) + ".json";
};

TEST_F(ConfigTest, LoadReturnsfalseForMissingFile)
{
	EXPECT_FALSE(Config::Instance().Load("no_such_file.json"));
}

TEST_F(ConfigTest, LoadReturnsTrueForValidFile)
{
	WriteJson(R"({"server": {"port": 12345}})");
	EXPECT_TRUE(Config::Instance().Load(m_path));
}

TEST_F(ConfigTest, ServerPortParsed)
{
	WriteJson(R"({"server": {"port": 8080}})");
	Config::Instance().Load(m_path);
	EXPECT_EQ(Config::Instance().GetServer().port, 8080);
}

TEST_F(ConfigTest, ServerDomainParsed)
{
	WriteJson(R"({"server": {"domain": "mail.example.com"}})");
	Config::Instance().Load(m_path);
	EXPECT_EQ(Config::Instance().GetServer().domain, "mail.example.com");
}

TEST_F(ConfigTest, ServerWorkerThreadsParsed)
{
	WriteJson(R"({"server": {"worker_threads": 8}})");
	Config::Instance().Load(m_path);
	EXPECT_EQ(Config::Instance().GetServer().worker_threads, 8);
}

TEST_F(ConfigTest, ImapPortParsed)
{
	WriteJson(R"({"imap": {"port": 993}})");
	Config::Instance().Load(m_path);
	EXPECT_EQ(Config::Instance().GetImap().port, 993);
}

TEST_F(ConfigTest, ImapTimeoutParsed)
{
	WriteJson(R"({"imap": {"timeout_mins": 60}})");
	Config::Instance().Load(m_path);
	EXPECT_EQ(Config::Instance().GetImap().timeout_mins, 60);
}

TEST_F(ConfigTest, LoggingLevelParsed)
{
	WriteJson(R"({"logging": {"log_level": "TRACE"}})");
	Config::Instance().Load(m_path);
	EXPECT_EQ(Config::Instance().GetLogging().log_level, "TRACE");
}

TEST_F(ConfigTest, LoggingFilePathParsed)
{
	WriteJson(R"({"logging": {"file_path": "custom.log"}})");
	Config::Instance().Load(m_path);
	EXPECT_EQ(Config::Instance().GetLogging().file_path, "custom.log");
}

TEST_F(ConfigTest, ProtoMaxLineSizeParsed)
{
	WriteJson(R"({"proto": {"max_line_size": 4096}})");
	Config::Instance().Load(m_path);
	EXPECT_EQ(Config::Instance().GetProto().max_line_size, 4096);
}

TEST_F(ConfigTest, DatabasePageLimitParsed)
{
	WriteJson(R"({"database": {"default_page_limit": 100}})");
	Config::Instance().Load(m_path);
	EXPECT_EQ(Config::Instance().GetDatabase().default_page_limit, 100);
}

TEST_F(ConfigTest, MimeHeaderChunkSizeParsed)
{
	WriteJson(R"({"mime": {"header_chunk_size": 30}})");
	Config::Instance().Load(m_path);
	EXPECT_EQ(Config::Instance().GetMime().header_chunk_size, 30);
}

TEST_F(ConfigTest, Base64MaxFileSizeParsed)
{
	WriteJson(R"({"base64": {"max_file_size_mb": 20}})");
	Config::Instance().Load(m_path);
	EXPECT_EQ(Config::Instance().GetBase64().max_file_size_mb, 20);
}

TEST_F(ConfigTest, InvalidPortFallsBackToDefault)
{
	WriteJson(R"({"server": {"port": 99999}})");
	Config::Instance().Load(m_path);
	EXPECT_LE(Config::Instance().GetServer().port, 65535);
}

TEST_F(ConfigTest, NonNumericPortFallsBackToDefault)
{
	WriteJson(R"({"server": {"port": "not_a_number"}})");
	Config::Instance().Load(m_path);
	EXPECT_GT(Config::Instance().GetServer().port, 0);
}

TEST_F(ConfigTest, FullConfigParsed)
{
	WriteJson(R"({
		"server": {"port": 2525, "domain": "test.local", "worker_threads": 2, "db_path": "test.db"},
		"imap": {"port": 1143, "db_path": "imap.db", "timeout_mins": 15},
		"logging": {"log_level": "NONE", "file_path": "test.log", "max_file_size_mb": 5},
		"proto": {"max_line_size": 1024, "socket_timeout_secs": 10, "max_message_size_mb": 5},
		"database": {"default_page_limit": 25},
		"mime": {"header_chunk_size": 60, "default_domain": "test.local"},
		"base64": {"max_file_size_mb": 10, "chunk_size_bytes": 1024, "line_length": 64}
	})");

	ASSERT_TRUE(Config::Instance().Load(m_path));

	EXPECT_EQ(Config::Instance().GetServer().port, 2525);
	EXPECT_EQ(Config::Instance().GetServer().domain, "test.local");
	EXPECT_EQ(Config::Instance().GetServer().worker_threads, 2);
	EXPECT_EQ(Config::Instance().GetServer().db_path, "test.db");
	EXPECT_EQ(Config::Instance().GetImap().port, 1143);
	EXPECT_EQ(Config::Instance().GetImap().timeout_mins, 15);
	EXPECT_EQ(Config::Instance().GetLogging().log_level, "NONE");
	EXPECT_EQ(Config::Instance().GetProto().max_line_size, 1024);
	EXPECT_EQ(Config::Instance().GetDatabase().default_page_limit, 25);
	EXPECT_EQ(Config::Instance().GetMime().header_chunk_size, 60);
	EXPECT_EQ(Config::Instance().GetBase64().max_file_size_mb, 10);
}
