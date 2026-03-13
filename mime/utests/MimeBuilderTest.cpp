#include <gtest/gtest.h>

#include "../include/Email.h"
#include "../include/MimeBuilder.h"
#include "../../logger/Logger.h"
#include "MockLogger.h"

using namespace SmtpClient;


// Helpers

static Email MakeValidEmail(const std::string& plain_text = "Hello World")
{
	Email e;
	e.sender     = "alice@example.com";
	e.recipient  = "bob@example.com";
	e.subject    = "Test Subject";
	e.plain_text = plain_text;
	return e;
}

static Logger MakeLogger()
{
	return Logger(std::make_unique<MockStrategy>());
}


// GenerateBoundary

TEST(MimeBuilderTest, GenerateBoundary_StartsWithSeparator)
{
	EXPECT_EQ(MimeBuilder::GenerateBoundary().substr(0, 11), "----=_Part_");
}

TEST(MimeBuilderTest, GenerateBoundary_IsUnique)
{
	EXPECT_NE(MimeBuilder::GenerateBoundary(), MimeBuilder::GenerateBoundary());
}


// Validation

TEST(MimeBuilderTest, BuildEmail_FailsWithEmptySender)
{
	auto logger = MakeLogger();
	Email e = MakeValidEmail();
	e.sender  = "";
	std::string mime;
	EXPECT_FALSE(MimeBuilder::BuildEmail(e, mime, logger));
}

TEST(MimeBuilderTest, BuildEmail_FailsWithEmptyRecipient)
{
	auto  logger = MakeLogger();
	Email e = MakeValidEmail();
	e.recipient = "";
	std::string mime;
	EXPECT_FALSE(MimeBuilder::BuildEmail(e, mime, logger));
}

TEST(MimeBuilderTest, BuildEmail_FailsWithEmptyBody)
{
	auto  logger = MakeLogger();
	Email e = MakeValidEmail();
	e.plain_text = "";
	std::string mime;
	EXPECT_FALSE(MimeBuilder::BuildEmail(e, mime, logger));
}

TEST(MimeBuilderTest, BuildEmail_FailsWithInvalidSender)
{
	auto  logger = MakeLogger();
	Email e = MakeValidEmail();
	e.sender = "invalid-email";
	std::string mime;
	EXPECT_FALSE(MimeBuilder::BuildEmail(e, mime, logger));
}


// Required headers

TEST(MimeBuilderTest, BuildEmail_ContainsMimeVersion)
{
	auto logger = MakeLogger();
	Email e = MakeValidEmail();
	std::string mime;
	ASSERT_TRUE(MimeBuilder::BuildEmail(e, mime, logger));
	EXPECT_NE(mime.find("MIME-Version: 1.0"), std::string::npos);
}

TEST(MimeBuilderTest, BuildEmail_ContainsDate)
{
	auto logger = MakeLogger();
	Email e = MakeValidEmail();
	std::string mime;
	ASSERT_TRUE(MimeBuilder::BuildEmail(e, mime, logger));
	EXPECT_NE(mime.find("Date:"), std::string::npos);
}

TEST(MimeBuilderTest, BuildEmail_ContainsFromAndTo)
{
	auto logger = MakeLogger();
	Email e = MakeValidEmail();
	std::string mime;
	ASSERT_TRUE(MimeBuilder::BuildEmail(e, mime, logger));
	EXPECT_NE(mime.find("From: alice@example.com"), std::string::npos);
	EXPECT_NE(mime.find("To: bob@example.com"), std::string::npos);
}

TEST(MimeBuilderTest, BuildEmail_ContainsMessageId)
{
	auto logger = MakeLogger();
	Email e = MakeValidEmail();
	std::string mime;
	ASSERT_TRUE(MimeBuilder::BuildEmail(e, mime, logger));
	EXPECT_NE(mime.find("Message-ID:"), std::string::npos);
}


// Subject encoding

TEST(MimeBuilderTest, BuildEmail_AsciiSubjectPassthrough)
{
	auto logger = MakeLogger();
	Email e = MakeValidEmail();
	e.subject  = "Hello";
	std::string mime;
	ASSERT_TRUE(MimeBuilder::BuildEmail(e, mime, logger));
	EXPECT_NE(mime.find("Subject: Hello"), std::string::npos);
}

TEST(MimeBuilderTest, BuildEmail_CyrillicSubjectEncoded)
{
	auto logger = MakeLogger();
	Email e = MakeValidEmail();
	e.subject = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD1\x96\xD1\x82";
	std::string mime;
	ASSERT_TRUE(MimeBuilder::BuildEmail(e, mime, logger));
	EXPECT_NE(mime.find("=?UTF-8?B?"), std::string::npos);
}


// Threading headers

TEST(MimeBuilderTest, BuildEmail_NoThreadingHeadersByDefault)
{
	auto logger = MakeLogger();
	Email e = MakeValidEmail();
	std::string mime;
	ASSERT_TRUE(MimeBuilder::BuildEmail(e, mime, logger));
	EXPECT_EQ(mime.find("In-Reply-To:"), std::string::npos);
	EXPECT_EQ(mime.find("References:"), std::string::npos);
}

TEST(MimeBuilderTest, BuildEmail_ThreadingHeadersPresent)
{
	auto logger = MakeLogger();
	Email e = MakeValidEmail();
	e.in_reply_to = "<orig@test.com>";
	e.references = "<orig@test.com>";
	std::string mime;
	ASSERT_TRUE(MimeBuilder::BuildEmail(e, mime, logger));
	EXPECT_NE(mime.find("In-Reply-To: <orig@test.com>"), std::string::npos);
	EXPECT_NE(mime.find("References: <orig@test.com>"), std::string::npos);
}


// Content structure

TEST(MimeBuilderTest, BuildEmail_PlainTextStructure)
{
	auto logger = MakeLogger();
	Email e = MakeValidEmail("Body text");
	std::string mime;
	ASSERT_TRUE(MimeBuilder::BuildEmail(e, mime, logger));
	EXPECT_NE(mime.find("Content-Type: text/plain"), std::string::npos);
	EXPECT_NE(mime.find("Body text"), std::string::npos);
	EXPECT_EQ(mime.find("multipart/mixed"), std::string::npos);
}

TEST(MimeBuilderTest, BuildEmail_HtmlCreatesAlternative)
{
	auto logger  = MakeLogger();
	Email e = MakeValidEmail("Plain text");
	e.html_text = "<p>Hello</p>";
	std::string mime;
	ASSERT_TRUE(MimeBuilder::BuildEmail(e, mime, logger));
	EXPECT_NE(mime.find("multipart/alternative"), std::string::npos);
	EXPECT_NE(mime.find("Content-Transfer-Encoding: base64"), std::string::npos);
}

TEST(MimeBuilderTest, BuildEmail_AttachmentCreatesMixed)
{
	auto logger = MakeLogger();
	Email e = MakeValidEmail("Plain text");
	e.AddAttachment("file.txt", "text/plain", "SGVsbG8=");
	std::string mime;
	ASSERT_TRUE(MimeBuilder::BuildEmail(e, mime, logger));
	EXPECT_NE(mime.find("multipart/mixed"), std::string::npos);
	EXPECT_NE(mime.find("Content-Disposition: attachment"), std::string::npos);
}

TEST(MimeBuilderTest, BuildEmail_AttachmentBase64DataPresent)
{
	auto logger = MakeLogger();
	Email e = MakeValidEmail("Body");
	e.AddAttachment("test.txt", "text/plain", "SGVsbG8=");
	std::string mime;
	ASSERT_TRUE(MimeBuilder::BuildEmail(e, mime, logger));
	EXPECT_NE(mime.find("SGVsbG8="), std::string::npos);
}

TEST(MimeBuilderTest, BuildEmail_HtmlAndAttachmentNested)
{
	auto logger = MakeLogger();
	Email e = MakeValidEmail("Plain text");
	e.html_text = "<p>HTML</p>";
	e.AddAttachment("file.txt", "text/plain", "SGVsbG8=");
	std::string mime;
	ASSERT_TRUE(MimeBuilder::BuildEmail(e, mime, logger));
	EXPECT_NE(mime.find("multipart/mixed"), std::string::npos);
	EXPECT_NE(mime.find("multipart/alternative"), std::string::npos);
	EXPECT_NE(mime.find("Content-Disposition: attachment"), std::string::npos);
}

TEST(MimeBuilderTest, BuildEmail_CyrillicFilenameEncoded)
{
	auto logger = MakeLogger();
	Email e = MakeValidEmail("Body");
	e.AddAttachment("\xD0\xB7\xD0\xB2\xD1\x96\xD1\x82", "text/plain", "SGVsbG8=");
	std::string mime;
	ASSERT_TRUE(MimeBuilder::BuildEmail(e, mime, logger));
	EXPECT_NE(mime.find("=?UTF-8?B?"), std::string::npos);
}
