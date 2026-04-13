#include <gtest/gtest.h>

#include "../include/MimeBuilder.h"
#include "../include/MimeComposer.h"
#include "../include/MimeParser.h"
#include "Logger.h"
#include "MockLogger.h"

using namespace SmtpClient;

static Logger MakeLogger()
{
	return Logger(std::make_unique<MockStrategy>());
}


// Helpers

static const std::string kSimpleRaw =
	"MIME-Version: 1.0\r\n"
	"Date: Mon, 01 Mar 2025 10:00:00 +0200\r\n"
	"Message-ID: <test.001@example.com>\r\n"
	"From: alice@example.com\r\n"
	"To: bob@example.com\r\n"
	"Subject: Hello\r\n"
	"Content-Type: text/plain; charset=\"utf-8\"\r\n"
	"\r\n"
	"Body text here.";


// Basic

TEST(MimeParserTest, ParseEmail_EmptyInput_ReturnsFalse)
{
	auto  logger = MakeLogger();
	Email email;
	EXPECT_FALSE(MimeParser::ParseEmail("", email, logger));
}

TEST(MimeParserTest, ParseEmail_SimpleTextEmail)
{
	auto  logger = MakeLogger();
	Email email;
	ASSERT_TRUE(MimeParser::ParseEmail(kSimpleRaw, email, logger));
	EXPECT_EQ(email.sender,  "alice@example.com");
	ASSERT_EQ(email.to.size(), 1u);
	EXPECT_EQ(email.to[0],   "bob@example.com");
	EXPECT_EQ(email.subject, "Hello");
	EXPECT_EQ(email.date,       "Mon, 01 Mar 2025 10:00:00 +0200");
	EXPECT_EQ(email.message_id, "<test.001@example.com>");
	EXPECT_NE(email.plain_text.find("Body text here."), std::string::npos);
}


// Threading headers

TEST(MimeParserTest, ParseEmail_ParsesThreadingHeaders)
{
	auto        logger = MakeLogger();
	Email       email;
	std::string raw =
		"MIME-Version: 1.0\r\n"
		"Date: Mon, 01 Mar 2025 10:00:00 +0200\r\n"
		"Message-ID: <reply.001@example.com>\r\n"
		"In-Reply-To: <original.001@example.com>\r\n"
		"References: <original.001@example.com>\r\n"
		"From: bob@example.com\r\n"
		"To: alice@example.com\r\n"
		"Subject: Re: Hello\r\n"
		"Content-Type: text/plain; charset=\"utf-8\"\r\n"
		"\r\n"
		"Reply body.";

	ASSERT_TRUE(MimeParser::ParseEmail(raw, email, logger));
	EXPECT_EQ(email.in_reply_to, "<original.001@example.com>");
	EXPECT_EQ(email.references,  "<original.001@example.com>");
}


// Case-insensitive headers

TEST(MimeParserTest, ParseEmail_CaseInsensitiveHeaders)
{
	auto        logger = MakeLogger();
	Email       email;
	std::string raw =
		"mime-version: 1.0\r\n"
		"from: alice@example.com\r\n"
		"to: bob@example.com\r\n"
		"subject: Test\r\n"
		"content-type: text/plain; charset=\"utf-8\"\r\n"
		"\r\n"
		"Body.";

	ASSERT_TRUE(MimeParser::ParseEmail(raw, email, logger));
	EXPECT_EQ(email.sender,  "alice@example.com");
	ASSERT_EQ(email.to.size(), 1u);
	EXPECT_EQ(email.to[0],   "bob@example.com");
	EXPECT_EQ(email.subject, "Test");
}


// Folded headers

TEST(MimeParserTest, ParseEmail_FoldedSubject)
{
	auto logger = MakeLogger();
	Email email;
	std::string raw =
		"MIME-Version: 1.0\r\n"
		"From: alice@example.com\r\n"
		"To: bob@example.com\r\n"
		"Subject: Part1\r\n"
		"\tPart2\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"Body.";

	ASSERT_TRUE(MimeParser::ParseEmail(raw, email, logger));
	EXPECT_NE(email.subject.find("Part1"), std::string::npos);
	EXPECT_NE(email.subject.find("Part2"), std::string::npos);
}


// Body encoding (simple email)

TEST(MimeParserTest, ParseEmail_QuotedPrintableBody)
{
	auto logger = MakeLogger();
	Email email;
	std::string raw =
		"MIME-Version: 1.0\r\n"
		"From: a@a.com\r\n"
		"To: b@b.com\r\n"
		"Content-Type: text/plain; charset=\"utf-8\"\r\n"
		"Content-Transfer-Encoding: quoted-printable\r\n"
		"\r\n"
		"=41=42=43";

	ASSERT_TRUE(MimeParser::ParseEmail(raw, email, logger));
	EXPECT_NE(email.plain_text.find("ABC"), std::string::npos);
}

TEST(MimeParserTest, ParseEmail_Base64Body)
{
	auto logger = MakeLogger();
	Email email;
	std::string raw =
		"MIME-Version: 1.0\r\n"
		"From: a@a.com\r\n"
		"To: b@b.com\r\n"
		"Content-Type: text/plain; charset=\"utf-8\"\r\n"
		"Content-Transfer-Encoding: base64\r\n"
		"\r\n"
		"SGVsbG8gV29ybGQ=";

	ASSERT_TRUE(MimeParser::ParseEmail(raw, email, logger));
	EXPECT_NE(email.plain_text.find("Hello World"), std::string::npos);
}


// Multipart/alternative

TEST(MimeParserTest, ParseEmail_MultipartAlternative)
{
	auto logger = MakeLogger();
	Email email;
	std::string raw =
		"MIME-Version: 1.0\r\n"
		"From: a@a.com\r\n"
		"To: b@b.com\r\n"
		"Content-Type: multipart/alternative; boundary=\"BOUND\"\r\n"
		"\r\n"
		"--BOUND\r\n"
		"Content-Type: text/plain; charset=\"utf-8\"\r\n"
		"\r\n"
		"Plain body\r\n"
		"--BOUND\r\n"
		"Content-Type: text/html; charset=\"utf-8\"\r\n"
		"\r\n"
		"<p>HTML body</p>\r\n"
		"--BOUND--\r\n";

	ASSERT_TRUE(MimeParser::ParseEmail(raw, email, logger));
	EXPECT_NE(email.plain_text.find("Plain body"), std::string::npos);
	EXPECT_NE(email.html_text.find("<p>HTML body</p>"), std::string::npos);
}


// Attachments

TEST(MimeParserTest, ParseEmail_AttachmentDetectedViaDisposition)
{
	auto logger = MakeLogger();
	Email email;
	std::string raw =
		"MIME-Version: 1.0\r\n"
		"From: a@a.com\r\n"
		"To: b@b.com\r\n"
		"Content-Type: multipart/mixed; boundary=\"BOUND\"\r\n"
		"\r\n"
		"--BOUND\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"Body text\r\n"
		"--BOUND\r\n"
		"Content-Type: application/octet-stream\r\n"
		"Content-Disposition: attachment; filename=\"test.bin\"\r\n"
		"Content-Transfer-Encoding: base64\r\n"
		"\r\n"
		"SGVsbG8=\r\n"
		"--BOUND--\r\n";

	ASSERT_TRUE(MimeParser::ParseEmail(raw, email, logger));
	ASSERT_EQ(email.attachments.size(), 1u);
	EXPECT_EQ(email.attachments[0].file_name, "test.bin");
}

TEST(MimeParserTest, ParseEmail_AttachmentDetectedViaName)
{
	auto logger = MakeLogger();
	Email email;
	std::string raw =
		"MIME-Version: 1.0\r\n"
		"From: a@a.com\r\n"
		"To: b@b.com\r\n"
		"Content-Type: multipart/mixed; boundary=\"BOUND\"\r\n"
		"\r\n"
		"--BOUND\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"Body\r\n"
		"--BOUND\r\n"
		"Content-Type: text/plain; name=\"notes.txt\"\r\n"
		"Content-Transfer-Encoding: base64\r\n"
		"\r\n"
		"SGVsbG8=\r\n"
		"--BOUND--\r\n";

	ASSERT_TRUE(MimeParser::ParseEmail(raw, email, logger));
	ASSERT_EQ(email.attachments.size(), 1u);
	EXPECT_EQ(email.attachments[0].file_name, "notes.txt");
}


// Round-trip integrity (MimeBuilder -> MimeParser)

TEST(MimeParserTest, RoundTrip_PlainText)
{
	auto logger = MakeLogger();
	Email original;
	original.sender     = "alice@example.com";
	original.to         = { "bob@example.com" };
	original.subject    = "Round-trip test";
	original.plain_text = "Hello from round-trip!";

	std::string mime;
	ASSERT_TRUE(MimeBuilder::BuildEmail(original, mime, logger));

	Email parsed;
	ASSERT_TRUE(MimeParser::ParseEmail(mime, parsed, logger));

	EXPECT_EQ(parsed.sender,  original.sender);
	ASSERT_EQ(parsed.to.size(), 1u);
	EXPECT_EQ(parsed.to[0],   original.to[0]);
	EXPECT_EQ(parsed.subject, original.subject);
	EXPECT_NE(parsed.plain_text.find(original.plain_text), std::string::npos);
}

TEST(MimeParserTest, RoundTrip_CyrillicSubject)
{
	auto  logger   = MakeLogger();
	Email original;
	original.sender     = "alice@example.com";
	original.to         = { "bob@example.com" };
	original.subject    = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD1\x96\xD1\x82";
	original.plain_text = "Body text";

	std::string mime;
	ASSERT_TRUE(MimeBuilder::BuildEmail(original, mime, logger));

	Email parsed;
	ASSERT_TRUE(MimeParser::ParseEmail(mime, parsed, logger));
	EXPECT_EQ(parsed.subject, original.subject);
}

TEST(MimeParserTest, RoundTrip_WithHtml)
{
	auto logger = MakeLogger();
	Email original;
	original.sender     = "alice@example.com";
	original.to         = { "bob@example.com" };
	original.subject    = "HTML test";
	original.plain_text = "Plain text";
	original.html_text  = "<p>HTML text</p>";

	std::string mime;
	ASSERT_TRUE(MimeBuilder::BuildEmail(original, mime, logger));

	Email parsed;
	ASSERT_TRUE(MimeParser::ParseEmail(mime, parsed, logger));
	EXPECT_TRUE(parsed.HasHtml());
	EXPECT_NE(parsed.html_text.find("<p>HTML text</p>"), std::string::npos);
}

TEST(MimeParserTest, RoundTrip_WithAttachment)
{
	auto logger = MakeLogger();
	Email original;
	original.sender     = "alice@example.com";
	original.to         = { "bob@example.com" };
	original.subject    = "Attachment test";
	original.plain_text = "See attachment";
	original.AddAttachment("hello.txt", "text/plain", "SGVsbG8gV29ybGQh");

	std::string mime;
	ASSERT_TRUE(MimeBuilder::BuildEmail(original, mime, logger));

	Email parsed;
	ASSERT_TRUE(MimeParser::ParseEmail(mime, parsed, logger));
	ASSERT_EQ(parsed.attachments.size(), 1u);
	EXPECT_EQ(parsed.attachments[0].file_name, "hello.txt");
	EXPECT_EQ(parsed.attachments[0].file_size, 12u);
}

TEST(MimeParserTest, RoundTrip_ThreadingHeaders)
{
	auto logger = MakeLogger();
	Email original;
	original.sender     = "alice@example.com";
	original.to         = { "bob@example.com" };
	original.subject    = "Thread test";
	original.plain_text   = "Body";
	original.message_id   = "<orig.001@test.com>";
	original.in_reply_to  = "<prev.001@test.com>";
	original.references   = "<prev.001@test.com>";

	std::string mime;
	ASSERT_TRUE(MimeBuilder::BuildEmail(original, mime, logger));

	Email parsed;
	ASSERT_TRUE(MimeParser::ParseEmail(mime, parsed, logger));
	EXPECT_EQ(parsed.in_reply_to, "<prev.001@test.com>");
	EXPECT_EQ(parsed.references,  "<prev.001@test.com>");
}

// ─────────────────────────────────────────────────────────────────────────────
// ParseEmail: error/edge paths
// ─────────────────────────────────────────────────────────────────────────────

TEST(MimeParserTest, ParseEmail_MultipartNoBoundaryReturnsFalse)
{
	auto logger = MakeLogger();
	// multipart/mixed with no boundary parameter
	std::string raw =
		"From: sender@example.com\r\n"
		"Content-Type: multipart/mixed\r\n"
		"\r\n"
		"Body without boundary";

	Email out;
	EXPECT_FALSE(MimeParser::ParseEmail(raw, out, logger));
}

TEST(MimeParserTest, ParseEmail_TopLevelBase64Body)
{
	auto logger = MakeLogger();
	// "Hello" in base64 is SGVsbG8=
	std::string raw =
		"From: sender@example.com\r\n"
		"Content-Type: text/plain; charset=\"utf-8\"\r\n"
		"Content-Transfer-Encoding: base64\r\n"
		"\r\n"
		"SGVsbG8=\r\n";

	Email out;
	ASSERT_TRUE(MimeParser::ParseEmail(raw, out, logger));
	EXPECT_NE(out.plain_text.find("Hello"), std::string::npos);
}

TEST(MimeParserTest, ParseEmail_TopLevelBase64InvalidDataFallback)
{
	auto logger = MakeLogger();
	std::string raw =
		"From: sender@example.com\r\n"
		"Content-Type: text/plain; charset=\"utf-8\"\r\n"
		"Content-Transfer-Encoding: base64\r\n"
		"\r\n"
		"!!!NOT_VALID_BASE64!!!\r\n";

	Email out;
	// Should not throw; parser falls back to raw body on decode error
	EXPECT_NO_THROW(MimeParser::ParseEmail(raw, out, logger));
}

TEST(MimeParserTest, ParseEmail_7bitBodyNoEncoding)
{
	auto logger = MakeLogger();
	std::string raw =
		"From: a@b.com\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"Plain 7bit body.";

	Email out;
	ASSERT_TRUE(MimeParser::ParseEmail(raw, out, logger));
	EXPECT_NE(out.plain_text.find("Plain 7bit body."), std::string::npos);
	EXPECT_EQ(out.plain_text_encoding, "7bit");
}

// ─────────────────────────────────────────────────────────────────────────────
// ParseStructure
// ─────────────────────────────────────────────────────────────────────────────

TEST(MimeParserTest, ParseStructure_EmptyInputReturnsFalse)
{
	auto logger = MakeLogger();
	MimePart root;
	EXPECT_FALSE(MimeParser::ParseStructure("", root, logger));
}

TEST(MimeParserTest, ParseStructure_SimplePlainText)
{
	auto logger = MakeLogger();
	std::string raw =
		"Content-Type: text/plain; charset=\"utf-8\"\r\n"
		"Content-Transfer-Encoding: 7bit\r\n"
		"\r\n"
		"Hello world";

	MimePart root;
	ASSERT_TRUE(MimeParser::ParseStructure(raw, root, logger));
	EXPECT_EQ(root.type,    "text");
	EXPECT_EQ(root.subtype, "plain");
	EXPECT_FALSE(root.IsMultipart());
}

TEST(MimeParserTest, ParseStructure_MultipartAlternative)
{
	auto logger = MakeLogger();
	std::string raw =
		"Content-Type: multipart/alternative; boundary=\"BOUND\"\r\n"
		"\r\n"
		"--BOUND\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"Plain part\r\n"
		"--BOUND\r\n"
		"Content-Type: text/html\r\n"
		"\r\n"
		"<p>HTML part</p>\r\n"
		"--BOUND--\r\n";

	MimePart root;
	ASSERT_TRUE(MimeParser::ParseStructure(raw, root, logger));
	EXPECT_TRUE(root.IsMultipart());
	EXPECT_EQ(root.children.size(), 2u);
	EXPECT_EQ(root.children[0].subtype, "plain");
	EXPECT_EQ(root.children[1].subtype, "html");
}

TEST(MimeParserTest, ParseStructure_MultipartNoBoundary)
{
	auto logger = MakeLogger();
	std::string raw =
		"Content-Type: multipart/mixed\r\n"
		"\r\n"
		"body without boundary";

	MimePart root;
	ASSERT_TRUE(MimeParser::ParseStructure(raw, root, logger));
	EXPECT_TRUE(root.IsMultipart());
	EXPECT_TRUE(root.children.empty());
}

TEST(MimeParserTest, ParseStructure_NoContentType)
{
	auto logger = MakeLogger();
	std::string raw =
		"From: a@b.com\r\n"
		"\r\n"
		"Body without content-type header";

	MimePart root;
	ASSERT_TRUE(MimeParser::ParseStructure(raw, root, logger));
	EXPECT_EQ(root.type,    "text");
	EXPECT_EQ(root.subtype, "plain");
}
