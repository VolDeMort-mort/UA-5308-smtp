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