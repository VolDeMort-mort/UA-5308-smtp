#include "ImapUtils.hpp"

#include <fstream>
#include <gtest/gtest.h>
#include <stdexcept>
#include <unistd.h>

#include "Entity/Message.h"
#include "Email.h"
#include "Logger.h"
#include "ConsoleStrategy.h"
#include "MimePart.h"

static std::string imapUtilsTmpFile(const std::string& suffix) {
    return "/tmp/imap_utils_test_" + std::to_string(getpid()) + "_" + suffix;
}

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

TEST(ImapUtilsTest, FormatFlagsResponse_NoFlags)
{
	Message m{};
	m.is_recent = false;
	auto r = IMAP_UTILS::FormatFlagsResponse(m);
	EXPECT_EQ(r, "(FLAGS ())");
}

TEST(ImapUtilsTest, FormatFlagsResponse_Seen)
{
	Message m{};
	m.is_recent = false;
	m.is_seen = true;
	auto r = IMAP_UTILS::FormatFlagsResponse(m);
	EXPECT_NE(r.find("\\Seen"), std::string::npos);
}

TEST(ImapUtilsTest, FormatFlagsResponse_Recent)
{
	Message m{};
	m.is_recent = true;
	auto r = IMAP_UTILS::FormatFlagsResponse(m);
	EXPECT_NE(r.find("\\Recent"), std::string::npos);
}

TEST(ImapUtilsTest, FormatFlagsResponse_MultipleFlags)
{
	Message m{};
	m.is_recent = false;
	m.is_seen = true;
	m.is_flagged = true;
	auto r = IMAP_UTILS::FormatFlagsResponse(m);
	EXPECT_NE(r.find("\\Seen"), std::string::npos);
	EXPECT_NE(r.find("\\Flagged"), std::string::npos);
}

TEST(ImapUtilsTest, FormatFlagsResponse_Deleted)
{
	Message m{};
	m.is_recent = false;
	m.is_deleted = true;
	auto r = IMAP_UTILS::FormatFlagsResponse(m);
	EXPECT_NE(r.find("\\Deleted"), std::string::npos);
}

TEST(ImapUtilsTest, FormatFlagsResponse_Draft)
{
	Message m{};
	m.is_recent = false;
	m.is_draft = true;
	auto r = IMAP_UTILS::FormatFlagsResponse(m);
	EXPECT_NE(r.find("\\Draft"), std::string::npos);
}

TEST(ImapUtilsTest, FormatFlagsResponse_Answered)
{
	Message m{};
	m.is_recent = false;
	m.is_answered = true;
	auto r = IMAP_UTILS::FormatFlagsResponse(m);
	EXPECT_NE(r.find("\\Answered"), std::string::npos);
}

TEST(ImapUtilsTest, GetBodyContent_MissingFile)
{
	Message m{};
	m.raw_file_path = "/tmp/nonexistent_file_xyz_12345.eml";
	auto result = IMAP_UTILS::GetBodyContent(m);
	EXPECT_EQ(result, "");
}

TEST(ImapUtilsTest, GetBodyContent_ExistingFile)
{
	std::string path = imapUtilsTmpFile("body.eml");
	{
		std::ofstream f(path);
		f << "Subject: Test\r\n\r\nHello body\r\n";
	}
	Message m{};
	m.raw_file_path = path;
	auto result = IMAP_UTILS::GetBodyContent(m);
	EXPECT_NE(result.find("Hello body"), std::string::npos);
	std::remove(path.c_str());
}

TEST(ImapUtilsTest, CombineSplitBodySections_PlainItems)
{
	std::vector<std::string> items = {"FLAGS", "UID", "RFC822.SIZE"};
	auto result = IMAP_UTILS::CombineSplitBodySections(items);
	ASSERT_EQ(result.size(), 3u);
	EXPECT_EQ(result[0], "FLAGS");
	EXPECT_EQ(result[1], "UID");
	EXPECT_EQ(result[2], "RFC822.SIZE");
}

TEST(ImapUtilsTest, CombineSplitBodySections_BodyHeaderFieldsSplit)
{
	std::vector<std::string> items = {"BODY[HEADER.FIELDS", "(From", "Subject)]"};
	auto result = IMAP_UTILS::CombineSplitBodySections(items);
	ASSERT_EQ(result.size(), 1u);
	EXPECT_EQ(result[0], "BODY[HEADER.FIELDS (From Subject)]");
}

TEST(ImapUtilsTest, CombineSplitBodySections_BodyPeekSplit)
{
	std::vector<std::string> items = {"BODY.PEEK[HEADER.FIELDS", "(From)]"};
	auto result = IMAP_UTILS::CombineSplitBodySections(items);
	ASSERT_EQ(result.size(), 1u);
	EXPECT_EQ(result[0], "BODY.PEEK[HEADER.FIELDS (From)]");
}

TEST(ImapUtilsTest, CombineSplitBodySections_AlreadyComplete)
{
	std::vector<std::string> items = {"BODY[HEADER.FIELDS (From Subject)]"};
	auto result = IMAP_UTILS::CombineSplitBodySections(items);
	ASSERT_EQ(result.size(), 1u);
	EXPECT_EQ(result[0], "BODY[HEADER.FIELDS (From Subject)]");
}

TEST(ImapUtilsTest, CombineSplitBodySections_Empty)
{
	std::vector<std::string> items = {};
	auto result = IMAP_UTILS::CombineSplitBodySections(items);
	EXPECT_TRUE(result.empty());
}

TEST(ImapUtilsTest, BuildBodystructureFromMimePart_TextLeaf)
{
	SmtpClient::MimePart part;
	part.type     = "text";
	part.subtype  = "plain";
	part.charset  = "utf-8";
	part.encoding = "7bit";
	part.size     = 100;
	part.lines    = 5;

	auto result = IMAP_UTILS::BuildBodystructureFromMimePart(part);
	EXPECT_NE(result.find("\"text\""), std::string::npos);
	EXPECT_NE(result.find("\"plain\""), std::string::npos);
	EXPECT_NE(result.find("\"utf-8\""), std::string::npos);
	EXPECT_NE(result.find("100"), std::string::npos);
}

TEST(ImapUtilsTest, BuildBodystructureFromMimePart_MultipartWithBoundary)
{
	SmtpClient::MimePart root;
	root.type     = "multipart";
	root.subtype  = "mixed";
	root.boundary = "boundary123";

	SmtpClient::MimePart child;
	child.type     = "text";
	child.subtype  = "plain";
	child.charset  = "utf-8";
	child.encoding = "7bit";
	child.size     = 50;
	child.lines    = 2;
	root.children.push_back(child);

	auto result = IMAP_UTILS::BuildBodystructureFromMimePart(root);
	EXPECT_NE(result.find("\"mixed\""), std::string::npos);
	EXPECT_NE(result.find("boundary123"), std::string::npos);
}

TEST(ImapUtilsTest, BuildBodystructureFromMimePart_MultipartNoBoundary)
{
	SmtpClient::MimePart root;
	root.type    = "multipart";
	root.subtype = "alternative";

	SmtpClient::MimePart child;
	child.type     = "text";
	child.subtype  = "html";
	child.charset  = "utf-8";
	child.encoding = "base64";
	child.size     = 200;
	child.lines    = 10;
	root.children.push_back(child);

	auto result = IMAP_UTILS::BuildBodystructureFromMimePart(root);
	EXPECT_NE(result.find("\"alternative\""), std::string::npos);
	EXPECT_NE(result.find("NIL"), std::string::npos);
}

TEST(ImapUtilsTest, GetBodySection_MissingFile)
{
	Message m{};
	m.raw_file_path = "/tmp/no_such_file_xyz.eml";
	EXPECT_EQ(IMAP_UTILS::GetBodySection(m, "HEADER"), "");
}

TEST(ImapUtilsTest, GetBodySection_Header)
{
	std::string path = imapUtilsTmpFile("section_hdr.eml");
	{
		std::ofstream f(path);
		f << "Subject: Hello\r\nFrom: a@b.com\r\n\r\nBody text here\r\n";
	}
	Message m{};
	m.raw_file_path = path;
	auto result = IMAP_UTILS::GetBodySection(m, "HEADER");
	EXPECT_NE(result.find("Subject: Hello"), std::string::npos);
	std::remove(path.c_str());
}

TEST(ImapUtilsTest, GetBodySection_Text)
{
	std::string path = imapUtilsTmpFile("section_txt.eml");
	{
		std::ofstream f(path);
		f << "Subject: Hello\r\nFrom: a@b.com\r\n\r\nBody text here\r\n";
	}
	Message m{};
	m.raw_file_path = path;
	auto result = IMAP_UTILS::GetBodySection(m, "TEXT");
	EXPECT_NE(result.find("Body text here"), std::string::npos);
	std::remove(path.c_str());
}

TEST(ImapUtilsTest, GetBodySection_Mime)
{
	std::string path = imapUtilsTmpFile("section_mime.eml");
	{
		std::ofstream f(path);
		f << "Subject: Hello\r\nFrom: a@b.com\r\n\r\nBody text here\r\n";
	}
	Message m{};
	m.raw_file_path = path;
	auto result = IMAP_UTILS::GetBodySection(m, "MIME");
	EXPECT_NE(result.find("Subject: Hello"), std::string::npos);
	std::remove(path.c_str());
}

TEST(ImapUtilsTest, GetBodySection_HeaderFields)
{
	std::string path = imapUtilsTmpFile("section_hdrfields.eml");
	{
		std::ofstream f(path);
		f << "Subject: Hello\r\nFrom: a@b.com\r\n\r\nBody text\r\n";
	}
	Message m{};
	m.raw_file_path = path;
	auto result = IMAP_UTILS::GetBodySection(m, "HEADER.FIELDS (Subject)");
	EXPECT_NE(result.find("Subject: Hello"), std::string::npos);
	std::remove(path.c_str());
}

TEST(ImapUtilsTest, GetBodySection_NumericPart1_NonMultipart)
{
	std::string path = imapUtilsTmpFile("section_num.eml");
	{
		std::ofstream f(path);
		f << "Subject: Hello\r\nContent-Type: text/plain\r\n\r\nSimple body\r\n";
	}
	Message m{};
	m.raw_file_path = path;
	auto result = IMAP_UTILS::GetBodySection(m, "1");
	EXPECT_NE(result.find("Simple body"), std::string::npos);
	std::remove(path.c_str());
}

TEST(ImapUtilsTest, GetBodySection_NumericPart2_NonMultipart_Empty)
{
	std::string path = imapUtilsTmpFile("section_num2.eml");
	{
		std::ofstream f(path);
		f << "Subject: Hello\r\nContent-Type: text/plain\r\n\r\nSimple body\r\n";
	}
	Message m{};
	m.raw_file_path = path;
	auto result = IMAP_UTILS::GetBodySection(m, "2");
	EXPECT_EQ(result, "");
	std::remove(path.c_str());
}

TEST(ImapUtilsTest, GetBodySection_UnknownSection_ReturnsRaw)
{
	std::string path = imapUtilsTmpFile("section_raw.eml");
	const std::string content = "Subject: Hello\r\nFrom: a@b.com\r\n\r\nBody\r\n";
	{
		std::ofstream f(path);
		f << content;
	}
	Message m{};
	m.raw_file_path = path;
	auto result = IMAP_UTILS::GetBodySection(m, "UNKNOWNSECTION");
	EXPECT_NE(result.find("Subject: Hello"), std::string::npos);
	std::remove(path.c_str());
}

TEST(ImapUtilsTest, GetParsedEmail_MissingFile)
{
	Logger logger(std::make_unique<ConsoleStrategy>());
	Message m{};
	m.raw_file_path = "/tmp/no_such_email_xyz.eml";
	auto result = IMAP_UTILS::GetParsedEmail(m, logger);
	EXPECT_FALSE(result.has_value());
}

TEST(ImapUtilsTest, GetParsedMimePart_MissingFile)
{
	Logger logger(std::make_unique<ConsoleStrategy>());
	Message m{};
	m.raw_file_path = "/tmp/no_such_mime_xyz.eml";
	auto result = IMAP_UTILS::GetParsedMimePart(m, logger);
	EXPECT_FALSE(result.has_value());
}

TEST(ImapUtilsTest, CommandTypeToString_UidAndOtherCommands)
{
	EXPECT_EQ(IMAP_UTILS::CommandTypeToString(ImapCommandType::UidFetch), "UID FETCH");
	EXPECT_EQ(IMAP_UTILS::CommandTypeToString(ImapCommandType::UidStore), "UID STORE");
	EXPECT_EQ(IMAP_UTILS::CommandTypeToString(ImapCommandType::UidCopy), "UID COPY");
	EXPECT_EQ(IMAP_UTILS::CommandTypeToString(ImapCommandType::Subscribe), "SUBSCRIBE");
	EXPECT_EQ(IMAP_UTILS::CommandTypeToString(ImapCommandType::Unsubscribe), "UNSUBSCRIBE");
	EXPECT_EQ(IMAP_UTILS::CommandTypeToString(ImapCommandType::Close), "CLOSE");
	EXPECT_EQ(IMAP_UTILS::CommandTypeToString(ImapCommandType::Check), "CHECK");
	EXPECT_EQ(IMAP_UTILS::CommandTypeToString(ImapCommandType::StartTLS), "STARTTLS");
	EXPECT_EQ(IMAP_UTILS::CommandTypeToString(ImapCommandType::Noop), "NOOP");
}

TEST(ImapUtilsTest, BuildBodystructure_NoEmailOpt)
{
	Message msg{};
	msg.size_bytes = 512;
	auto result = IMAP_UTILS::BuildBodystructure(msg, std::nullopt, std::nullopt);
	EXPECT_NE(result.find("\"text\""), std::string::npos);
	EXPECT_NE(result.find("\"plain\""), std::string::npos);
	EXPECT_NE(result.find("512"), std::string::npos);
}

TEST(ImapUtilsTest, BuildBodystructure_PlainOnly)
{
	Message msg{};
	SmtpClient::Email email;
	email.plain_text = "Hello plain";
	email.plain_text_encoding = "7bit";
	email.charset = "utf-8";
	auto result = IMAP_UTILS::BuildBodystructure(msg, email, std::nullopt);
	EXPECT_NE(result.find("\"text\""), std::string::npos);
	EXPECT_NE(result.find("\"plain\""), std::string::npos);
}

TEST(ImapUtilsTest, BuildBodystructure_WithHtml)
{
	Message msg{};
	SmtpClient::Email email;
	email.plain_text = "Plain part";
	email.plain_text_encoding = "7bit";
	email.html_text = "<b>HTML</b>";
	email.html_text_encoding = "7bit";
	email.charset = "utf-8";
	email.boundary_alternative = "alt-boundary";
	auto result = IMAP_UTILS::BuildBodystructure(msg, email, std::nullopt);
	EXPECT_NE(result.find("\"alternative\""), std::string::npos);
	EXPECT_NE(result.find("alt-boundary"), std::string::npos);
}

TEST(ImapUtilsTest, BuildBodystructure_WithHtmlEmptyBoundary)
{
	Message msg{};
	SmtpClient::Email email;
	email.plain_text = "Plain";
	email.plain_text_encoding = "7bit";
	email.html_text = "<b>HTML</b>";
	email.html_text_encoding = "7bit";
	email.charset = "utf-8";
	auto result = IMAP_UTILS::BuildBodystructure(msg, email, std::nullopt);
	EXPECT_NE(result.find("\"alternative\""), std::string::npos);
}

TEST(ImapUtilsTest, BuildBodystructure_WithAttachments)
{
	Message msg{};
	SmtpClient::Email email;
	email.plain_text = "Plain part";
	email.plain_text_encoding = "7bit";
	email.charset = "utf-8";
	email.boundary_mixed = "mix-boundary";
	email.attachments.emplace_back("file.txt", "text/plain", "aGVsbG8=");
	auto result = IMAP_UTILS::BuildBodystructure(msg, email, std::nullopt);
	EXPECT_NE(result.find("\"mixed\""), std::string::npos);
	EXPECT_NE(result.find("mix-boundary"), std::string::npos);
	EXPECT_NE(result.find("file.txt"), std::string::npos);
}

TEST(ImapUtilsTest, BuildBodystructure_WithHtmlAndAttachments)
{
	Message msg{};
	SmtpClient::Email email;
	email.plain_text = "Plain";
	email.plain_text_encoding = "7bit";
	email.html_text = "<b>HTML</b>";
	email.html_text_encoding = "7bit";
	email.charset = "utf-8";
	email.boundary_alternative = "alt-bound";
	email.boundary_mixed = "mix-bound";
	email.attachments.emplace_back("doc.pdf", "application/pdf", "aGVsbG8=");
	auto result = IMAP_UTILS::BuildBodystructure(msg, email, std::nullopt);
	EXPECT_NE(result.find("\"mixed\""), std::string::npos);
	EXPECT_NE(result.find("doc.pdf"), std::string::npos);
}

TEST(ImapUtilsTest, GetBodySection_NumericPart1_HEADER_Suffix)
{
	std::string path = imapUtilsTmpFile("sec_1hdr.eml");
	{
		std::ofstream f(path);
		f << "Subject: Hello\r\nContent-Type: text/plain\r\n\r\nBody text\r\n";
	}
	Message m{};
	m.raw_file_path = path;
	auto result = IMAP_UTILS::GetBodySection(m, "1.HEADER");
	EXPECT_NE(result.find("Subject: Hello"), std::string::npos);
	std::remove(path.c_str());
}

TEST(ImapUtilsTest, GetBodySection_NumericPart1_MIME_Suffix)
{
	std::string path = imapUtilsTmpFile("sec_1mime.eml");
	{
		std::ofstream f(path);
		f << "Subject: Hello\r\nContent-Type: text/plain\r\n\r\nBody text\r\n";
	}
	Message m{};
	m.raw_file_path = path;
	auto result = IMAP_UTILS::GetBodySection(m, "1.MIME");
	EXPECT_NE(result.find("Subject: Hello"), std::string::npos);
	std::remove(path.c_str());
}

TEST(ImapUtilsTest, GetBodySection_Multipart_Part1)
{
	std::string path = imapUtilsTmpFile("mp1.eml");
	{
		std::ofstream f(path);
		f << "Subject: Multi\r\n"
		  << "Content-Type: multipart/mixed; boundary=\"bound1\"\r\n"
		  << "\r\n"
		  << "--bound1\r\n"
		  << "Content-Type: text/plain\r\n"
		  << "\r\n"
		  << "Part one body\r\n"
		  << "--bound1--\r\n";
	}
	Message m{};
	m.raw_file_path = path;
	auto result = IMAP_UTILS::GetBodySection(m, "1");
	EXPECT_NE(result.find("Part one body"), std::string::npos);
	std::remove(path.c_str());
}

TEST(ImapUtilsTest, GetBodySection_Multipart_Part1_HEADER)
{
	std::string path = imapUtilsTmpFile("mp1hdr.eml");
	{
		std::ofstream f(path);
		f << "Subject: Multi\r\n"
		  << "Content-Type: multipart/mixed; boundary=\"bound2\"\r\n"
		  << "\r\n"
		  << "--bound2\r\n"
		  << "Content-Type: text/plain\r\n"
		  << "\r\n"
		  << "Part header body\r\n"
		  << "--bound2--\r\n";
	}
	Message m{};
	m.raw_file_path = path;
	auto result = IMAP_UTILS::GetBodySection(m, "1.HEADER");
	EXPECT_NE(result.find("Content-Type"), std::string::npos);
	std::remove(path.c_str());
}

TEST(ImapUtilsTest, GetBodySection_Multipart_Part1_MIME)
{
	std::string path = imapUtilsTmpFile("mp1mime.eml");
	{
		std::ofstream f(path);
		f << "Subject: Multi\r\n"
		  << "Content-Type: multipart/mixed; boundary=\"bound3\"\r\n"
		  << "\r\n"
		  << "--bound3\r\n"
		  << "Content-Type: text/plain\r\n"
		  << "\r\n"
		  << "Part mime body\r\n"
		  << "--bound3--\r\n";
	}
	Message m{};
	m.raw_file_path = path;
	auto result = IMAP_UTILS::GetBodySection(m, "1.MIME");
	EXPECT_NE(result.find("Content-Type"), std::string::npos);
	std::remove(path.c_str());
}

TEST(ImapUtilsTest, GetBodySection_Multipart_Part1_TEXT)
{
	std::string path = imapUtilsTmpFile("mp1txt.eml");
	{
		std::ofstream f(path);
		f << "Subject: Multi\r\n"
		  << "Content-Type: multipart/mixed; boundary=\"bound4\"\r\n"
		  << "\r\n"
		  << "--bound4\r\n"
		  << "Content-Type: text/plain\r\n"
		  << "\r\n"
		  << "Text section body\r\n"
		  << "--bound4--\r\n";
	}
	Message m{};
	m.raw_file_path = path;
	auto result = IMAP_UTILS::GetBodySection(m, "1.TEXT");
	EXPECT_NE(result.find("Text section body"), std::string::npos);
	std::remove(path.c_str());
}

TEST(ImapUtilsTest, GetBodySection_Multipart_PartNotFound)
{
	std::string path = imapUtilsTmpFile("mp_notfound.eml");
	{
		std::ofstream f(path);
		f << "Subject: Multi\r\n"
		  << "Content-Type: multipart/mixed; boundary=\"bound5\"\r\n"
		  << "\r\n"
		  << "--bound5\r\n"
		  << "Content-Type: text/plain\r\n"
		  << "\r\n"
		  << "Only part\r\n"
		  << "--bound5--\r\n";
	}
	Message m{};
	m.raw_file_path = path;
	auto result = IMAP_UTILS::GetBodySection(m, "99");
	EXPECT_EQ(result, "");
	std::remove(path.c_str());
}
