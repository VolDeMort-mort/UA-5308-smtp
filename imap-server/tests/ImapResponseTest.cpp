#include "ImapResponse.hpp"

#include <gtest/gtest.h>

using namespace ImapResponse;

TEST(ImapResponseTest, OkWithMessage)
{
	auto result = Ok("A001", "Login successful");
	EXPECT_EQ(result, "A001 OK Login successful\r\n");
}

TEST(ImapResponseTest, OkEmptyMessage)
{
	auto result = Ok("A001", "");
	EXPECT_EQ(result, "A001 OK \r\n");
}

TEST(ImapResponseTest, NoWithMessage)
{
	auto result = No("A001", "Login failed");
	EXPECT_EQ(result, "A001 NO Login failed\r\n");
}

TEST(ImapResponseTest, NoEmptyMessage)
{
	auto result = No("A001", "");
	EXPECT_EQ(result, "A001 NO \r\n");
}

TEST(ImapResponseTest, BadWithMessage)
{
	auto result = Bad("A001", "Command not recognized");
	EXPECT_EQ(result, "A001 BAD Command not recognized\r\n");
}

TEST(ImapResponseTest, BadEmptyMessage)
{
	auto result = Bad("A001", "");
	EXPECT_EQ(result, "A001 BAD \r\n");
}

TEST(ImapResponseTest, UntaggedWithMessage)
{
	auto result = Untagged("LIST (\\HasNoChildren) \"/\" \"INBOX\"");
	EXPECT_EQ(result, "* LIST (\\HasNoChildren) \"/\" \"INBOX\"\r\n");
}

TEST(ImapResponseTest, Capability)
{
	auto result = Capability();
	EXPECT_EQ(result, "* CAPABILITY IMAP4rev1 LOGIN LOGOUT SELECT LIST LSUB STATUS FETCH STORE CREATE DELETE RENAME "
					  "COPY EXPUNGE\r\n");
}

TEST(ImapResponseTest, FlagsDefault)
{
	auto result = Flags();
	EXPECT_EQ(result, "* FLAGS (\\Seen \\Answered \\Flagged \\Draft \\Recent)\r\n");
}

TEST(ImapResponseTest, FlagsCustom)
{
	auto result = Flags("(\\Seen \\Answered)");
	EXPECT_EQ(result, "* FLAGS (\\Seen \\Answered)\r\n");
}

TEST(ImapResponseTest, ExistsZero)
{
	auto result = Exists(0);
	EXPECT_EQ(result, "* 0 EXISTS\r\n");
}

TEST(ImapResponseTest, ExistsPositive)
{
	auto result = Exists(100);
	EXPECT_EQ(result, "* 100 EXISTS\r\n");
}

TEST(ImapResponseTest, RecentZero)
{
	auto result = Recent(0);
	EXPECT_EQ(result, "* 0 RECENT\r\n");
}

TEST(ImapResponseTest, RecentPositive)
{
	auto result = Recent(5);
	EXPECT_EQ(result, "* 5 RECENT\r\n");
}

TEST(ImapResponseTest, ListDefault)
{
	auto result = List('/', "INBOX", "");
	EXPECT_EQ(result, "* LIST (\\) \"/\" \"INBOX\"\r\n");
}

TEST(ImapResponseTest, ListWithAttributes)
{
	auto result = List('/', "INBOX", "HasNoChildren");
	EXPECT_EQ(result, "* LIST (\\HasNoChildren) \"/\" \"INBOX\"\r\n");
}

TEST(ImapResponseTest, Fetch)
{
	auto result = Fetch(1, "(FLAGS (\\Seen) RFC822.TEXT {4}\r\ntest)");
	EXPECT_EQ(result, "* 1 FETCH (FLAGS (\\Seen) RFC822.TEXT {4}\r\ntest)\r\n");
}

TEST(ImapResponseTest, FetchMultipleMessages)
{
	auto result = Fetch(10, "(FLAGS (\\Seen))");
	EXPECT_EQ(result, "* 10 FETCH (FLAGS (\\Seen))\r\n");
}
