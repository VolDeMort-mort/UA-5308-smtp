#include "ImapUtils.hpp"

#include <gtest/gtest.h>
#include <stdexcept>

#include "Entity/Message.h"

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
		{"COPY", ImapCommandType::Copy},
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
		{ImapCommandType::Copy, "COPY"},
		{ImapCommandType::Expunge, "EXPUNGE"},
		{ImapCommandType::Unknown, "UNKNOWN"}};

	for (const auto& [input_type, expected_string] : test_cases)
	{
		auto result = IMAP_UTILS::CommandTypeToString(input_type);
		EXPECT_EQ(result, expected_string) << "Failed on expected string: " << expected_string;
	}
}

TEST(ImapUtilsTest, TrimParentheses_Empty)
{
	EXPECT_EQ(IMAP_UTILS::TrimParentheses(""), "");
}

TEST(ImapUtilsTest, TrimParentheses_Basic)
{
	EXPECT_EQ(IMAP_UTILS::TrimParentheses("(hello)"), "hello");
}

TEST(ImapUtilsTest, TrimParentheses_NoParentheses)
{
	EXPECT_EQ(IMAP_UTILS::TrimParentheses("hello"), "hello");
}

TEST(ImapUtilsTest, TrimParentheses_UnmatchedLeft)
{
	EXPECT_EQ(IMAP_UTILS::TrimParentheses("(hello"), "(hello");
}

TEST(ImapUtilsTest, TrimParentheses_UnmatchedRight)
{
	EXPECT_EQ(IMAP_UTILS::TrimParentheses("hello)"), "hello)");
}

TEST(ImapUtilsTest, TrimParentheses_Nested)
{
	EXPECT_EQ(IMAP_UTILS::TrimParentheses("((hello))"), "(hello)");
}

TEST(ImapUtilsTest, ParseSequenceSet_SingleNumber)
{
	auto result = IMAP_UTILS::ParseSequenceSet("5", 10);
	std::vector<int64_t> expected = {5};
	EXPECT_EQ(result, expected);
}

TEST(ImapUtilsTest, ParseSequenceSet_CommaSeparated)
{
	auto result = IMAP_UTILS::ParseSequenceSet("1,3,5", 10);
	std::vector<int64_t> expected = {1, 3, 5};
	EXPECT_EQ(result, expected);
}

TEST(ImapUtilsTest, ParseSequenceSet_Range)
{
	auto result = IMAP_UTILS::ParseSequenceSet("2:4", 10);
	std::vector<int64_t> expected = {2, 3, 4};
	EXPECT_EQ(result, expected);
}

TEST(ImapUtilsTest, ParseSequenceSet_ReverseRange)
{
	auto result = IMAP_UTILS::ParseSequenceSet("4:2", 10);
	std::vector<int64_t> expected = {2, 3, 4};
	EXPECT_EQ(result, expected);
}

TEST(ImapUtilsTest, ParseSequenceSet_StarSingle)
{
	auto result = IMAP_UTILS::ParseSequenceSet("*", 10);
	std::vector<int64_t> expected = {10};
	EXPECT_EQ(result, expected);
}

TEST(ImapUtilsTest, ParseSequenceSet_StarRange)
{
	auto result = IMAP_UTILS::ParseSequenceSet("8:*", 10);
	std::vector<int64_t> expected = {8, 9, 10};
	EXPECT_EQ(result, expected);
}

TEST(ImapUtilsTest, ParseSequenceSet_MixedAndOverlapping)
{
	auto result = IMAP_UTILS::ParseSequenceSet("1,3:5,4:6,*", 8);
	std::vector<int64_t> expected = {1, 3, 4, 5, 6, 8};
	EXPECT_EQ(result, expected);
}

TEST(ImapUtilsTest, SortMessagesByTimeDescending_Basic)
{
	Message m1;
	m1.internal_date = "2026-03-01 10:00:00";
	Message m2;
	m2.internal_date = "2026-03-02 10:00:00";
	Message m3;
	m3.internal_date = "2026-03-03 10:00:00";
	std::vector<Message> msgs = {m2, m1, m3};

	IMAP_UTILS::SortMessagesByTimeDescending(msgs);

	ASSERT_EQ(msgs.size(), 3);
	EXPECT_EQ(msgs[0].internal_date, "2026-03-03 10:00:00");
	EXPECT_EQ(msgs[1].internal_date, "2026-03-02 10:00:00");
	EXPECT_EQ(msgs[2].internal_date, "2026-03-01 10:00:00");
}

TEST(ImapUtilsTest, SortMessagesByTimeDescending_Empty)
{
	std::vector<Message> empty_msgs;
	IMAP_UTILS::SortMessagesByTimeDescending(empty_msgs);
	EXPECT_TRUE(empty_msgs.empty());
}

TEST(ImapUtilsTest, SortMessagesByTimeDescending_Single)
{
	Message m1;
	m1.internal_date = "2026-03-01 10:00:00";
	std::vector<Message> single_msg = {m1};
	IMAP_UTILS::SortMessagesByTimeDescending(single_msg);
	ASSERT_EQ(single_msg.size(), 1);
	EXPECT_EQ(single_msg[0].internal_date, "2026-03-01 10:00:00");
}

TEST(ImapUtilsTest, DateToISO_EmptyString)
{
	EXPECT_THROW(IMAP_UTILS::DateToEmlDate(""), std::runtime_error);
}

TEST(ImapUtilsTest, DateToISO_ValidDate_January)
{
	auto result = IMAP_UTILS::DateToEmlDate("2025-01-15 14:30:45");
	EXPECT_EQ(result, "Wed, 15 Jan 2025 14:30:45 +0000");
}

TEST(ImapUtilsTest, DateToISO_ValidDate_December)
{
	auto result = IMAP_UTILS::DateToEmlDate("2025-12-25 00:00:00");
	EXPECT_EQ(result, "Thu, 25 Dec 2025 00:00:00 +0000");
}

TEST(ImapUtilsTest, DateToISO_ValidDate_FebruaryLeapYear)
{
	auto result = IMAP_UTILS::DateToEmlDate("2024-02-29 23:59:59");
	EXPECT_EQ(result, "Thu, 29 Feb 2024 23:59:59 +0000");
}

TEST(ImapUtilsTest, DateToISO_ValidDate_Sunday)
{
	auto result = IMAP_UTILS::DateToEmlDate("2025-06-15 12:00:00");
	EXPECT_EQ(result, "Sun, 15 Jun 2025 12:00:00 +0000");
}

TEST(ImapUtilsTest, DateToISO_ValidDate_Monday)
{
	auto result = IMAP_UTILS::DateToEmlDate("2025-06-16 08:30:00");
	EXPECT_EQ(result, "Mon, 16 Jun 2025 08:30:00 +0000");
}

TEST(ImapUtilsTest, DateToISO_InvalidMonth_Zero)
{
	EXPECT_THROW(IMAP_UTILS::DateToEmlDate("2025-00-15 14:30:45"), std::runtime_error);
}

TEST(ImapUtilsTest, DateToISO_InvalidMonth_Thirteen)
{
	EXPECT_THROW(IMAP_UTILS::DateToEmlDate("2025-13-15 14:30:45"), std::runtime_error);
}

TEST(ImapUtilsTest, DateToISO_InvalidDay_Zero)
{
	EXPECT_THROW(IMAP_UTILS::DateToEmlDate("2025-01-00 14:30:45"), std::runtime_error);
}

TEST(ImapUtilsTest, DateToISO_InvalidDay_ThirtyTwo)
{
	EXPECT_THROW(IMAP_UTILS::DateToEmlDate("2025-01-32 14:30:45"), std::runtime_error);
}

TEST(ImapUtilsTest, DateToISO_InvalidHour)
{
	EXPECT_THROW(IMAP_UTILS::DateToEmlDate("2025-01-15 25:30:45"), std::runtime_error);
}

TEST(ImapUtilsTest, DateToISO_InvalidMinute)
{
	EXPECT_THROW(IMAP_UTILS::DateToEmlDate("2025-01-15 14:60:45"), std::runtime_error);
}

TEST(ImapUtilsTest, DateToISO_InvalidSecond)
{
	EXPECT_THROW(IMAP_UTILS::DateToEmlDate("2025-01-15 14:30:60"), std::runtime_error);
}

TEST(ImapUtilsTest, DateToISO_InvalidFormat)
{
	EXPECT_THROW(IMAP_UTILS::DateToEmlDate("not-a-date"), std::runtime_error);
}

TEST(ImapUtilsTest, DateToISO_MalformedInput)
{
	EXPECT_THROW(IMAP_UTILS::DateToEmlDate("2025-01-15"), std::runtime_error);
}

TEST(ImapUtilsTest, DateToIMAPInternal_EmptyString)
{
	EXPECT_THROW(IMAP_UTILS::DateToIMAPInternal(""), std::runtime_error);
}

TEST(ImapUtilsTest, DateToIMAPInternal_ValidDate_Basic)
{
	auto result = IMAP_UTILS::DateToIMAPInternal("2025-01-15 14:30:45");
	EXPECT_EQ(result, "15-Jan-2025 14:30:45 +0000");
}

TEST(ImapUtilsTest, DateToIMAPInternal_ValidDate_December)
{
	auto result = IMAP_UTILS::DateToIMAPInternal("2025-12-25 00:00:00");
	EXPECT_EQ(result, "25-Dec-2025 00:00:00 +0000");
}

TEST(ImapUtilsTest, DateToIMAPInternal_ValidDate_SingleDigitDay)
{
	auto result = IMAP_UTILS::DateToIMAPInternal("2025-06-05 08:30:00");
	EXPECT_EQ(result, "05-Jun-2025 08:30:00 +0000");
}

TEST(ImapUtilsTest, DateToIMAPInternal_ValidDate_SingleDigitMonth)
{
	auto result = IMAP_UTILS::DateToIMAPInternal("2025-06-15 08:30:00");
	EXPECT_EQ(result, "15-Jun-2025 08:30:00 +0000");
}

TEST(ImapUtilsTest, DateToIMAPInternal_InvalidMonth_Zero)
{
	EXPECT_THROW(IMAP_UTILS::DateToIMAPInternal("2025-00-15 14:30:45"), std::runtime_error);
}

TEST(ImapUtilsTest, DateToIMAPInternal_InvalidMonth_Thirteen)
{
	EXPECT_THROW(IMAP_UTILS::DateToIMAPInternal("2025-13-15 14:30:45"), std::runtime_error);
}

TEST(ImapUtilsTest, DateToIMAPInternal_InvalidDay_Zero)
{
	EXPECT_THROW(IMAP_UTILS::DateToIMAPInternal("2025-01-00 14:30:45"), std::runtime_error);
}

TEST(ImapUtilsTest, DateToIMAPInternal_InvalidDay_ThirtyTwo)
{
	EXPECT_THROW(IMAP_UTILS::DateToIMAPInternal("2025-01-32 14:30:45"), std::runtime_error);
}

TEST(ImapUtilsTest, DateToIMAPInternal_InvalidFormat)
{
	EXPECT_THROW(IMAP_UTILS::DateToIMAPInternal("not-a-date"), std::runtime_error);
}

TEST(ImapUtilsTest, DateToIMAPInternal_MalformedInput)
{
	EXPECT_THROW(IMAP_UTILS::DateToIMAPInternal("2025-01-15"), std::runtime_error);
}