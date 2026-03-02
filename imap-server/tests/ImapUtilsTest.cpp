#include "ImapUtils.hpp"

#include <gtest/gtest.h>

TEST(ImapUtilsTest, ToUpperEmpty)
{
	auto result = IMAP_UTILS::ToUpper("");
	EXPECT_EQ(result, "");
}

TEST(ImapUtilsTest, ToUpperLowercase)
{
	auto result = IMAP_UTILS::ToUpper("hello");
	EXPECT_EQ(result, "HELLO");
}

TEST(ImapUtilsTest, ToUpperUppercase)
{
	auto result = IMAP_UTILS::ToUpper("HELLO");
	EXPECT_EQ(result, "HELLO");
}

TEST(ImapUtilsTest, ToUpperMixedCase)
{
	auto result = IMAP_UTILS::ToUpper("HeLLo WoRLd");
	EXPECT_EQ(result, "HELLO WORLD");
}

TEST(ImapUtilsTest, ToUpperNumbers)
{
	auto result = IMAP_UTILS::ToUpper("123abc");
	EXPECT_EQ(result, "123ABC");
}

TEST(ImapUtilsTest, ToUpperSpecialChars)
{
	auto result = IMAP_UTILS::ToUpper("test@#$%");
	EXPECT_EQ(result, "TEST@#$%");
}

TEST(ImapUtilsTest, SplitArgsEmpty)
{
	auto result = IMAP_UTILS::SplitArgs("");
	EXPECT_TRUE(result.empty());
}

TEST(ImapUtilsTest, SplitArgsSingleArg)
{
	auto result = IMAP_UTILS::SplitArgs("hello");
	ASSERT_EQ(result.size(), 1);
	EXPECT_EQ(result[0], "hello");
}

TEST(ImapUtilsTest, SplitArgsMultipleArgs)
{
	auto result = IMAP_UTILS::SplitArgs("hello world test");
	ASSERT_EQ(result.size(), 3);
	EXPECT_EQ(result[0], "hello");
	EXPECT_EQ(result[1], "world");
	EXPECT_EQ(result[2], "test");
}

TEST(ImapUtilsTest, SplitArgsWithExtraSpaces)
{
	auto result = IMAP_UTILS::SplitArgs("  hello   world  ");
	ASSERT_EQ(result.size(), 2);
	EXPECT_EQ(result[0], "hello");
	EXPECT_EQ(result[1], "world");
}

TEST(ImapUtilsTest, SplitArgsWithNumbers)
{
	auto result = IMAP_UTILS::SplitArgs("A001 LOGIN user pass");
	ASSERT_EQ(result.size(), 4);
	EXPECT_EQ(result[0], "A001");
	EXPECT_EQ(result[1], "LOGIN");
	EXPECT_EQ(result[2], "user");
	EXPECT_EQ(result[3], "pass");
}

TEST(ImapUtilsTest, JoinArgsEmpty)
{
	auto result = IMAP_UTILS::JoinArgs({});
	EXPECT_EQ(result, "");
}

TEST(ImapUtilsTest, JoinArgsSingleArg)
{
	auto result = IMAP_UTILS::JoinArgs({"hello"});
	EXPECT_EQ(result, "hello");
}

TEST(ImapUtilsTest, JoinArgsMultipleArgs)
{
	auto result = IMAP_UTILS::JoinArgs({"hello", "world", "test"});
	EXPECT_EQ(result, "hello, world, test");
}

TEST(ImapUtilsTest, StringToCommandType)
{
	const std::vector<std::pair<std::string, ImapCommandType>> test_cases = {
		{"LOGIN", ImapCommandType::Login},
		{"LOGOUT", ImapCommandType::Logout},
		{"CAPABILITY", ImapCommandType::Capability},
		{"SELECT", ImapCommandType::Select},
		{"LIST", ImapCommandType::List},
		{"LSUB", ImapCommandType::Lsub},
		{"STATUS", ImapCommandType::Status},
		{"FETCH", ImapCommandType::Fetch},
		{"STORE", ImapCommandType::Store},
		{"CREATE", ImapCommandType::Create},
		{"DELETE", ImapCommandType::Delete},
		{"RENAME", ImapCommandType::Rename},
		{"IDLE", ImapCommandType::Idle},
		{"COPY", ImapCommandType::Copy},
		{"MOVE", ImapCommandType::Move},
		{"EXPUNGE", ImapCommandType::Expunge},
		{"UNKNOWNCMD", ImapCommandType::Unknown},
		{"login", ImapCommandType::Login}};

	for (const auto& [input_string, expected_type] : test_cases)
	{
		auto result = IMAP_UTILS::StringToCommandType(input_string);
		EXPECT_EQ(result, expected_type) << "Failed on string input: " << input_string;
	}
}

TEST(ImapUtilsTest, CommandTypeToString)
{
	const std::vector<std::pair<ImapCommandType, std::string>> test_cases = {
		{ImapCommandType::Login, "LOGIN"},
		{ImapCommandType::Logout, "LOGOUT"},
		{ImapCommandType::Capability, "CAPABILITY"},
		{ImapCommandType::Select, "SELECT"},
		{ImapCommandType::List, "LIST"},
		{ImapCommandType::Lsub, "LSUB"},
		{ImapCommandType::Status, "STATUS"},
		{ImapCommandType::Fetch, "FETCH"},
		{ImapCommandType::Store, "STORE"},
		{ImapCommandType::Create, "CREATE"},
		{ImapCommandType::Delete, "DELETE"},
		{ImapCommandType::Rename, "RENAME"},
		{ImapCommandType::Idle, "IDLE"},
		{ImapCommandType::Copy, "COPY"},
		{ImapCommandType::Move, "MOVE"},
		{ImapCommandType::Expunge, "EXPUNGE"},
		{ImapCommandType::Unknown, "UNKNOWN"}};

	for (const auto& [input_type, expected_string] : test_cases)
	{
		auto result = IMAP_UTILS::CommandTypeToString(input_type);
		EXPECT_EQ(result, expected_string) << "Failed on expected string: " << expected_string;
	}
}