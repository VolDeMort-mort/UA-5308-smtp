#include "ImapCommand.hpp"

#include <gtest/gtest.h>

TEST(ImapCommandTest, DefaultConstructedCommand)
{
	ImapCommand cmd;
	EXPECT_TRUE(cmd.m_tag.empty());
	EXPECT_EQ(cmd.m_type, ImapCommandType::Unknown);
	EXPECT_TRUE(cmd.m_args.empty());
}

TEST(ImapCommandTest, ConstructedWithValues)
{
	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Login;
	cmd.m_args = {"user", "pass"};

	EXPECT_EQ(cmd.m_tag, "A001");
	EXPECT_EQ(cmd.m_type, ImapCommandType::Login);
	EXPECT_EQ(cmd.m_args.size(), 2);
	EXPECT_EQ(cmd.m_args[0], "user");
	EXPECT_EQ(cmd.m_args[1], "pass");
}

TEST(ImapCommandTest, CommandTypeEnumValues)
{
	EXPECT_EQ(static_cast<int>(ImapCommandType::Login), 0);
	EXPECT_EQ(static_cast<int>(ImapCommandType::Logout), 1);
	EXPECT_EQ(static_cast<int>(ImapCommandType::Capability), 2);
	EXPECT_EQ(static_cast<int>(ImapCommandType::Noop), 3);
	EXPECT_EQ(static_cast<int>(ImapCommandType::Select), 4);
	EXPECT_EQ(static_cast<int>(ImapCommandType::List), 5);
	EXPECT_EQ(static_cast<int>(ImapCommandType::Lsub), 6);
	EXPECT_EQ(static_cast<int>(ImapCommandType::Status), 7);
	EXPECT_EQ(static_cast<int>(ImapCommandType::Fetch), 8);
	EXPECT_EQ(static_cast<int>(ImapCommandType::Store), 9);
	EXPECT_EQ(static_cast<int>(ImapCommandType::Create), 10);
	EXPECT_EQ(static_cast<int>(ImapCommandType::Delete), 11);
	EXPECT_EQ(static_cast<int>(ImapCommandType::Rename), 12);
	EXPECT_EQ(static_cast<int>(ImapCommandType::Idle), 13);
	EXPECT_EQ(static_cast<int>(ImapCommandType::Copy), 14);
	EXPECT_EQ(static_cast<int>(ImapCommandType::Move), 15);
	EXPECT_EQ(static_cast<int>(ImapCommandType::Expunge), 16);
	EXPECT_EQ(static_cast<int>(ImapCommandType::UidFetch), 17);
	EXPECT_EQ(static_cast<int>(ImapCommandType::UidStore), 18);
	EXPECT_EQ(static_cast<int>(ImapCommandType::UidCopy), 19);
	EXPECT_EQ(static_cast<int>(ImapCommandType::Subscribe), 20);
	EXPECT_EQ(static_cast<int>(ImapCommandType::Unsubscribe), 21);
	EXPECT_EQ(static_cast<int>(ImapCommandType::Close), 22);
	EXPECT_EQ(static_cast<int>(ImapCommandType::Check), 23);
	EXPECT_EQ(static_cast<int>(ImapCommandType::Unknown), 24);
}
