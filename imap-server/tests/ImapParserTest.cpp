#include "ImapParser.hpp"

#include <gtest/gtest.h>

TEST(ImapParserTest, ParseLoginCommand)
{
	auto cmd = ImapParser::Parse("A001 LOGIN user pass");

	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Login;
	EXPECT_EQ(cmd.m_args.size(), 2);
	EXPECT_EQ(cmd.m_args[0], "user");
	EXPECT_EQ(cmd.m_args[1], "pass");
}

TEST(ImapParserTest, ParseSelectCommand)
{
	auto cmd = ImapParser::Parse("A002 SELECT INBOX");

	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Select;
	EXPECT_EQ(cmd.m_args.size(), 1);
	EXPECT_EQ(cmd.m_args[0], "INBOX");
}

TEST(ImapParserTest, ParseLogoutCommand)
{
	auto cmd = ImapParser::Parse("A003 LOGOUT");

	cmd.m_tag = "A003";
	cmd.m_type = ImapCommandType::Logout;
	EXPECT_TRUE(cmd.m_args.empty());
}

TEST(ImapParserTest, ParseCapabilityCommand)
{
	auto cmd = ImapParser::Parse("A004 CAPABILITY");

	cmd.m_tag = "A004";
	cmd.m_type = ImapCommandType::Capability;
	EXPECT_TRUE(cmd.m_args.empty());
}

TEST(ImapParserTest, ParseNoopCommand)
{
	auto cmd = ImapParser::Parse("A005 NOOP");

	cmd.m_tag = "A005";
	cmd.m_type = ImapCommandType::Noop;
	EXPECT_TRUE(cmd.m_args.empty());
}

TEST(ImapParserTest, ParseListCommand)
{
	auto cmd = ImapParser::Parse("A006 LIST \"\" *");

	cmd.m_tag = "A006";
	cmd.m_type = ImapCommandType::List;
	EXPECT_EQ(cmd.m_args.size(), 2);
	EXPECT_EQ(cmd.m_args[0], "");
	EXPECT_EQ(cmd.m_args[1], "*");
}

TEST(ImapParserTest, ParseStatusCommand)
{
	auto cmd = ImapParser::Parse("A007 STATUS INBOX (MESSAGES UNSEEN)");

	cmd.m_tag = "A007";
	cmd.m_type = ImapCommandType::Status;
	EXPECT_EQ(cmd.m_args.size(), 2);
	EXPECT_EQ(cmd.m_args[0], "INBOX");
	EXPECT_EQ(cmd.m_args[1], "(MESSAGES UNSEEN)");
}

TEST(ImapParserTest, ParseFetchCommand)
{
	auto cmd = ImapParser::Parse("A008 FETCH 1:10 (FLAGS RFC822.HEADER)");

	cmd.m_tag = "A008";
	cmd.m_type = ImapCommandType::Fetch;
	EXPECT_EQ(cmd.m_args.size(), 2);
	EXPECT_EQ(cmd.m_args[0], "1:10");
	EXPECT_EQ(cmd.m_args[1], "(FLAGS RFC822.HEADER)");
}

TEST(ImapParserTest, ParseFetchWithQuotedString)
{
	auto cmd = ImapParser::Parse("A022 FETCH 1 (BODY[\"TEXT\"])");

	cmd.m_type = ImapCommandType::Fetch;
	EXPECT_EQ(cmd.m_args.size(), 2);
	EXPECT_EQ(cmd.m_args[0], "1");
	EXPECT_EQ(cmd.m_args[1], "(BODY[\"TEXT\"])");
}

TEST(ImapParserTest, ParseStoreCommand)
{
	auto cmd = ImapParser::Parse("A009 STORE 1 +FLAGS (\\Seen)");

	cmd.m_tag = "A009";
	cmd.m_type = ImapCommandType::Store;
	EXPECT_EQ(cmd.m_args.size(), 3);
	EXPECT_EQ(cmd.m_args[0], "1");
	EXPECT_EQ(cmd.m_args[1], "+FLAGS");
	EXPECT_EQ(cmd.m_args[2], "(\\Seen)");
}

TEST(ImapParserTest, ParseCreateCommand)
{
	auto cmd = ImapParser::Parse("A010 CREATE NewMailbox");

	cmd.m_tag = "A010";
	cmd.m_type = ImapCommandType::Create;
	EXPECT_EQ(cmd.m_args.size(), 1);
	EXPECT_EQ(cmd.m_args[0], "NewMailbox");
}

TEST(ImapParserTest, ParseDeleteCommand)
{
	auto cmd = ImapParser::Parse("A011 DELETE OldMailbox");

	cmd.m_tag = "A011";
	cmd.m_type = ImapCommandType::Delete;
	EXPECT_EQ(cmd.m_args.size(), 1);
	EXPECT_EQ(cmd.m_args[0], "OldMailbox");
}

TEST(ImapParserTest, ParseRenameCommand)
{
	auto cmd = ImapParser::Parse("A012 RENAME OldName NewName");

	cmd.m_tag = "A012";
	cmd.m_type = ImapCommandType::Rename;
	EXPECT_EQ(cmd.m_args.size(), 2);
	EXPECT_EQ(cmd.m_args[0], "OldName");
	EXPECT_EQ(cmd.m_args[1], "NewName");
}

TEST(ImapParserTest, ParseCopyCommand)
{
	auto cmd = ImapParser::Parse("A013 COPY 1:10 Sent");

	cmd.m_tag = "A013";
	cmd.m_type = ImapCommandType::Copy;
	EXPECT_EQ(cmd.m_args.size(), 2);
	EXPECT_EQ(cmd.m_args[0], "1:10");
	EXPECT_EQ(cmd.m_args[1], "Sent");
}

TEST(ImapParserTest, ParseExpungeCommand)
{
	auto cmd = ImapParser::Parse("A014 EXPUNGE");

	cmd.m_tag = "A014";
	cmd.m_type = ImapCommandType::Expunge;
	EXPECT_TRUE(cmd.m_args.empty());
}

TEST(ImapParserTest, ParseEmptyLine)
{
	auto cmd = ImapParser::Parse("");

	EXPECT_TRUE(cmd.m_tag.empty());
	EXPECT_EQ(cmd.m_type, ImapCommandType::Unknown);
	EXPECT_TRUE(cmd.m_args.empty());
}

TEST(ImapParserTest, ParseCommandOnlyNoTag)
{
	auto cmd = ImapParser::Parse("LOGIN");

	EXPECT_TRUE(cmd.m_tag.empty());
	EXPECT_EQ(cmd.m_type, ImapCommandType::Unknown);
}

TEST(ImapParserTest, ParseUnknownCommand)
{
	auto cmd = ImapParser::Parse("A015 UNKNOWNCMD arg");

	cmd.m_tag = "A015";
	EXPECT_EQ(cmd.m_type, ImapCommandType::Unknown);
}

TEST(ImapParserTest, ParseCaseInsensitive)
{
	auto cmd = ImapParser::Parse("a001 login user pass");

	EXPECT_EQ(cmd.m_tag, "a001");
	EXPECT_EQ(cmd.m_type, ImapCommandType::Login);
}

TEST(ImapParserTest, ParseLowercaseCommand)
{
	auto cmd = ImapParser::Parse("A001 logout");

	EXPECT_EQ(cmd.m_type, ImapCommandType::Logout);
}

TEST(ImapParserTest, ParseMixedCaseCommand)
{
	auto cmd = ImapParser::Parse("A001 Select INBOX");

	EXPECT_EQ(cmd.m_type, ImapCommandType::Select);
}

TEST(ImapParserTest, ParseLsubCommand)
{
	auto cmd = ImapParser::Parse("A016 LSUB \"\" *");

	EXPECT_EQ(cmd.m_tag, "A016");
	EXPECT_EQ(cmd.m_type, ImapCommandType::Lsub);
	EXPECT_EQ(cmd.m_args.size(), 2);
}

TEST(ImapParserTest, ParseNestedParentheses)
{
	auto cmd = ImapParser::Parse("A017 FETCH 1 (FLAGS (\\Seen \\Answer))");

	EXPECT_EQ(cmd.m_type, ImapCommandType::Fetch);
	EXPECT_EQ(cmd.m_args.size(), 2);
	EXPECT_EQ(cmd.m_args[0], "1");
	EXPECT_EQ(cmd.m_args[1], "(FLAGS (\\Seen \\Answer))");
}

TEST(ImapParserTest, ParseLiteral)
{
	auto cmd = ImapParser::Parse("A019 APPEND INBOX {12}");

	EXPECT_EQ(cmd.m_type, ImapCommandType::Unknown);
	EXPECT_EQ(cmd.m_args.size(), 2);
	EXPECT_EQ(cmd.m_args[0], "INBOX");
	EXPECT_EQ(cmd.m_args[1], "{12}");
}

TEST(ImapParserTest, ParseQuotedString)
{
	auto cmd = ImapParser::Parse("A020 LOGIN \"test user\" \"pass word\"");

	EXPECT_EQ(cmd.m_type, ImapCommandType::Login);
	EXPECT_EQ(cmd.m_args.size(), 2);
	EXPECT_EQ(cmd.m_args[0], "test user");
	EXPECT_EQ(cmd.m_args[1], "pass word");
}

TEST(ImapParserTest, ParseListWithResponseCode)
{
	auto cmd = ImapParser::Parse("A021 LIST () \"\" *");

	EXPECT_EQ(cmd.m_type, ImapCommandType::List);
	EXPECT_EQ(cmd.m_args.size(), 3);
	EXPECT_EQ(cmd.m_args[0], "()");
	EXPECT_EQ(cmd.m_args[1], "");
	EXPECT_EQ(cmd.m_args[2], "*");
}
