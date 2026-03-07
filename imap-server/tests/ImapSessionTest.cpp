#include "ImapSession.hpp"

#include <gtest/gtest.h>

TEST(MailboxStateTest, DefaultConstructed)
{
	MailboxState mailbox;
	EXPECT_EQ(mailbox.m_name, "");
	EXPECT_EQ(mailbox.m_exists, 0);
	EXPECT_EQ(mailbox.m_recent, 0);
	EXPECT_EQ(mailbox.m_unseen, 0);
	EXPECT_TRUE(mailbox.m_flags.empty());
	EXPECT_FALSE(mailbox.m_readOnly);
}

TEST(SessionStateTest, EnumValues)
{
	EXPECT_EQ(static_cast<int>(SessionState::NonAuthenticated), 0);
	EXPECT_EQ(static_cast<int>(SessionState::Authenticated), 1);
	EXPECT_EQ(static_cast<int>(SessionState::Selected), 2);
}

TEST(MailboxStateTest, FlagsOperations)
{
	MailboxState mailbox;
	mailbox.m_flags.insert("\\Seen");
	mailbox.m_flags.insert("\\Answered");
	mailbox.m_flags.insert("\\Flagged");

	EXPECT_EQ(mailbox.m_flags.size(), 3);
	EXPECT_TRUE(mailbox.m_flags.count("\\Seen") > 0);
	EXPECT_TRUE(mailbox.m_flags.count("\\Answered") > 0);
	EXPECT_TRUE(mailbox.m_flags.count("\\Flagged") > 0);
	EXPECT_FALSE(mailbox.m_flags.count("\\Draft") > 0);
}
