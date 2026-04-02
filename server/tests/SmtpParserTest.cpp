#include "SmtpParser.hpp"

#include <gtest/gtest.h>

#include "SmtpCommand.hpp"

// ================================================================
//  HELO / EHLO
// ================================================================

TEST(SmtpParserTest, ParseHelo)
{
	auto cmd = SmtpParser::Parse("HELO client.example.com");
	EXPECT_EQ(cmd.type, SmtpCommandType::HELO);
	EXPECT_EQ(cmd.argument, "client.example.com");
}

TEST(SmtpParserTest, ParseHeloLowercase)
{
	auto cmd = SmtpParser::Parse("helo client.example.com");
	EXPECT_EQ(cmd.type, SmtpCommandType::HELO);
	EXPECT_EQ(cmd.argument, "client.example.com");
}

TEST(SmtpParserTest, ParseHeloMixedCase)
{
	auto cmd = SmtpParser::Parse("HeLo client.example.com");
	EXPECT_EQ(cmd.type, SmtpCommandType::HELO);
	EXPECT_EQ(cmd.argument, "client.example.com");
}

TEST(SmtpParserTest, ParseHeloTrimsArgument)
{
	auto cmd = SmtpParser::Parse("HELO   client.example.com  ");
	EXPECT_EQ(cmd.type, SmtpCommandType::HELO);
	EXPECT_EQ(cmd.argument, "client.example.com");
}

TEST(SmtpParserTest, ParseEhlo)
{
	auto cmd = SmtpParser::Parse("EHLO client.example.com");
	EXPECT_EQ(cmd.type, SmtpCommandType::EHLO);
	EXPECT_EQ(cmd.argument, "client.example.com");
}

TEST(SmtpParserTest, ParseEhloLowercase)
{
	auto cmd = SmtpParser::Parse("ehlo mail.test");
	EXPECT_EQ(cmd.type, SmtpCommandType::EHLO);
	EXPECT_EQ(cmd.argument, "mail.test");
}

// ================================================================
//  MAIL FROM
// ================================================================

TEST(SmtpParserTest, ParseMailFromWithAngleBrackets)
{
	auto cmd = SmtpParser::Parse("MAIL FROM:<sender@example.com>");
	EXPECT_EQ(cmd.type, SmtpCommandType::MAIL);
	EXPECT_EQ(cmd.argument, "sender@example.com");
}

TEST(SmtpParserTest, ParseMailFromWithoutAngleBrackets)
{
	auto cmd = SmtpParser::Parse("MAIL FROM:sender@example.com");
	EXPECT_EQ(cmd.type, SmtpCommandType::MAIL);
	EXPECT_EQ(cmd.argument, "sender@example.com");
}

TEST(SmtpParserTest, ParseMailFromLowercase)
{
	auto cmd = SmtpParser::Parse("mail from:<user@test.org>");
	EXPECT_EQ(cmd.type, SmtpCommandType::MAIL);
	EXPECT_EQ(cmd.argument, "user@test.org");
}

TEST(SmtpParserTest, ParseMailFromTrimsWhitespace)
{
	auto cmd = SmtpParser::Parse("MAIL FROM:  <user@test.org>  ");
	EXPECT_EQ(cmd.type, SmtpCommandType::MAIL);
	EXPECT_EQ(cmd.argument, "user@test.org");
}

TEST(SmtpParserTest, ParseMailFromEmptyAddress)
{
	auto cmd = SmtpParser::Parse("MAIL FROM:<>");
	EXPECT_EQ(cmd.type, SmtpCommandType::MAIL);
	EXPECT_EQ(cmd.argument, "");
}

// ================================================================
//  RCPT TO
// ================================================================

TEST(SmtpParserTest, ParseRcptToWithAngleBrackets)
{
	auto cmd = SmtpParser::Parse("RCPT TO:<recipient@example.com>");
	EXPECT_EQ(cmd.type, SmtpCommandType::RCPT);
	EXPECT_EQ(cmd.argument, "recipient@example.com");
}

TEST(SmtpParserTest, ParseRcptToWithoutAngleBrackets)
{
	auto cmd = SmtpParser::Parse("RCPT TO:recipient@example.com");
	EXPECT_EQ(cmd.type, SmtpCommandType::RCPT);
	EXPECT_EQ(cmd.argument, "recipient@example.com");
}

TEST(SmtpParserTest, ParseRcptToLowercase)
{
	auto cmd = SmtpParser::Parse("rcpt to:<bob@mail.com>");
	EXPECT_EQ(cmd.type, SmtpCommandType::RCPT);
	EXPECT_EQ(cmd.argument, "bob@mail.com");
}

// ================================================================
//  Simple commands (no argument)
// ================================================================

TEST(SmtpParserTest, ParseData)
{
	auto cmd = SmtpParser::Parse("DATA");
	EXPECT_EQ(cmd.type, SmtpCommandType::DATA);
	EXPECT_TRUE(cmd.argument.empty());
}

TEST(SmtpParserTest, ParseDataLowercase)
{
	auto cmd = SmtpParser::Parse("data");
	EXPECT_EQ(cmd.type, SmtpCommandType::DATA);
}

TEST(SmtpParserTest, ParseRset)
{
	auto cmd = SmtpParser::Parse("RSET");
	EXPECT_EQ(cmd.type, SmtpCommandType::RSET);
	EXPECT_TRUE(cmd.argument.empty());
}

TEST(SmtpParserTest, ParseNoop)
{
	auto cmd = SmtpParser::Parse("NOOP");
	EXPECT_EQ(cmd.type, SmtpCommandType::NOOP);
	EXPECT_TRUE(cmd.argument.empty());
}

TEST(SmtpParserTest, ParseQuit)
{
	auto cmd = SmtpParser::Parse("QUIT");
	EXPECT_EQ(cmd.type, SmtpCommandType::QUIT);
	EXPECT_TRUE(cmd.argument.empty());
}

TEST(SmtpParserTest, ParseStarttls)
{
	auto cmd = SmtpParser::Parse("STARTTLS");
	EXPECT_EQ(cmd.type, SmtpCommandType::STARTTLS);
	EXPECT_TRUE(cmd.argument.empty());
}

TEST(SmtpParserTest, ParseStarttlsLowercase)
{
	auto cmd = SmtpParser::Parse("starttls");
	EXPECT_EQ(cmd.type, SmtpCommandType::STARTTLS);
}

// ================================================================
//  AUTH
// ================================================================

TEST(SmtpParserTest, ParseAuthLoginNoCredentials)
{
	auto cmd = SmtpParser::Parse("AUTH LOGIN");
	EXPECT_EQ(cmd.type, SmtpCommandType::AUTH);
	EXPECT_EQ(cmd.argument, "LOGIN");
}

TEST(SmtpParserTest, ParseAuthPlainNoCredentials)
{
	auto cmd = SmtpParser::Parse("AUTH PLAIN");
	EXPECT_EQ(cmd.type, SmtpCommandType::AUTH);
	EXPECT_EQ(cmd.argument, "PLAIN");
}

TEST(SmtpParserTest, ParseAuthPlainInlineCredentials)
{
	// The mechanism is uppercased but the base64 blob preserves case.
	auto cmd = SmtpParser::Parse("AUTH PLAIN AGFsaWNlAHBhc3MxMjM=");
	EXPECT_EQ(cmd.type, SmtpCommandType::AUTH);
	EXPECT_EQ(cmd.argument, "PLAIN AGFsaWNlAHBhc3MxMjM=");
}

TEST(SmtpParserTest, ParseAuthLoginLowercaseMechanism)
{
	// Mechanism should be uppercased even when sent in lowercase.
	auto cmd = SmtpParser::Parse("AUTH login");
	EXPECT_EQ(cmd.type, SmtpCommandType::AUTH);
	EXPECT_EQ(cmd.argument, "LOGIN");
}

TEST(SmtpParserTest, ParseAuthPlainLowercaseMechanism)
{
	auto cmd = SmtpParser::Parse("auth plain AbCdEfGh==");
	EXPECT_EQ(cmd.type, SmtpCommandType::AUTH);
	EXPECT_EQ(cmd.argument, "PLAIN AbCdEfGh==");
}

// ================================================================
//  CRLF / edge cases
// ================================================================

TEST(SmtpParserTest, ParseStripsTrailingCarriageReturn)
{
	// Lines may arrive with \r\n; the parser should strip the \r.
	auto cmd = SmtpParser::Parse("QUIT\r");
	EXPECT_EQ(cmd.type, SmtpCommandType::QUIT);
}

TEST(SmtpParserTest, ParseHeloWithCarriageReturn)
{
	auto cmd = SmtpParser::Parse("HELO client.test\r");
	EXPECT_EQ(cmd.type, SmtpCommandType::HELO);
	EXPECT_EQ(cmd.argument, "client.test");
}

TEST(SmtpParserTest, ParseEmptyLineReturnsUnknown)
{
	auto cmd = SmtpParser::Parse("");
	EXPECT_EQ(cmd.type, SmtpCommandType::UNKNOWN);
}

TEST(SmtpParserTest, ParseUnknownCommandReturnsUnknown)
{
	auto cmd = SmtpParser::Parse("VRFY user@example.com");
	EXPECT_EQ(cmd.type, SmtpCommandType::UNKNOWN);
}

TEST(SmtpParserTest, ParseGibberishReturnsUnknown)
{
	auto cmd = SmtpParser::Parse("NOTACOMMAND");
	EXPECT_EQ(cmd.type, SmtpCommandType::UNKNOWN);
}