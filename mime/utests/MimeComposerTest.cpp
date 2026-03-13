#include <gtest/gtest.h>

#include "../include/MimeBuilder.h"
#include "../include/MimeComposer.h"
#include "../../logger/Logger.h"
#include "MockLogger.h"

using namespace SmtpClient;

static Logger MakeLogger()
{
	return Logger(std::make_unique<MockStrategy>());
}


// Helpers

static Email MakeOriginal()
{
	Email e;
	e.sender = "charlie@example.com";
	e.recipient = "alice@example.com";
	e.subject = "Question";
	e.date = "Tue, 04 Mar 2025 09:00:00 +0200";
	e.message_id = "<original.001@example.com>";
	e.plain_text = "Can you explain how this works?";
	return e;
}


// CreateNew

TEST(MimeComposerTest, CreateNew_SetsBasicFields)
{
	Email e = MimeComposer::CreateNew("alice@example.com", "bob@example.com",
									 "Hello", "Body text");
	EXPECT_EQ(e.sender, "alice@example.com");
	EXPECT_EQ(e.recipient, "bob@example.com");
	EXPECT_EQ(e.subject, "Hello");
	EXPECT_EQ(e.plain_text, "Body text");
}

TEST(MimeComposerTest, CreateNew_DefaultEmptyHtml)
{
	Email e = MimeComposer::CreateNew("a@a.com", "b@b.com", "Sub", "Body");
	EXPECT_FALSE(e.HasHtml());
}

TEST(MimeComposerTest, CreateNew_DefaultEmptyAttachments)
{
	Email e = MimeComposer::CreateNew("a@a.com", "b@b.com", "Sub", "Body");
	EXPECT_FALSE(e.HasAttachments());
}

TEST(MimeComposerTest, CreateNew_DefaultEmptyThreading)
{
	Email e = MimeComposer::CreateNew("a@a.com", "b@b.com", "Sub", "Body");
	EXPECT_TRUE(e.message_id.empty());
	EXPECT_TRUE(e.in_reply_to.empty());
	EXPECT_TRUE(e.references.empty());
}


// CreateReply

TEST(MimeComposerTest, CreateReply_RecipientIsOriginalSender)
{
	Email original = MakeOriginal();
	Email reply = MimeComposer::CreateReply(original, "alice@example.com", "Sure!");
	EXPECT_EQ(reply.recipient, original.sender);
}

TEST(MimeComposerTest, CreateReply_SenderSetCorrectly)
{
	Email original = MakeOriginal();
	Email reply = MimeComposer::CreateReply(original, "alice@example.com", "Sure!");
	EXPECT_EQ(reply.sender, "alice@example.com");
}

TEST(MimeComposerTest, CreateReply_SubjectHasRePrefix)
{
	Email original = MakeOriginal();
	Email reply = MimeComposer::CreateReply(original, "alice@example.com", "Body");
	EXPECT_EQ(reply.subject.substr(0, 4), "Re: ");
}

TEST(MimeComposerTest, CreateReply_DoesNotDoubleRePrefix)
{
	Email original = MakeOriginal();
	original.subject = "Re: Question";
	Email reply = MimeComposer::CreateReply(original, "alice@example.com", "Body");
	EXPECT_EQ(reply.subject.find("Re: Re:"), std::string::npos);
}

TEST(MimeComposerTest, CreateReply_CaseInsensitiveRePrefix)
{
	Email original = MakeOriginal();
	original.subject = "re: Question";
	Email reply = MimeComposer::CreateReply(original, "alice@example.com", "Body");
	EXPECT_EQ(reply.subject.find("Re: Re:"), std::string::npos);
}

TEST(MimeComposerTest, CreateReply_InReplyToSet)
{
	Email original = MakeOriginal();
	Email reply = MimeComposer::CreateReply(original, "alice@example.com", "Body");
	EXPECT_EQ(reply.in_reply_to, original.message_id);
}

TEST(MimeComposerTest, CreateReply_ReferencesSetFromMessageId)
{
	Email original = MakeOriginal();
	Email reply = MimeComposer::CreateReply(original, "alice@example.com", "Body");
	EXPECT_NE(reply.references.find(original.message_id), std::string::npos);
}

TEST(MimeComposerTest, CreateReply_ReferencesChained)
{
	Email original = MakeOriginal();
	original.references = "<prev.001@example.com>";
	Email reply = MimeComposer::CreateReply(original, "alice@example.com", "Body");
	EXPECT_NE(reply.references.find("<prev.001@example.com>"), std::string::npos);
	EXPECT_NE(reply.references.find(original.message_id), std::string::npos);
}

TEST(MimeComposerTest, CreateReply_BodyContainsReplyText)
{
	Email original = MakeOriginal();
	Email reply = MimeComposer::CreateReply(original, "alice@example.com", "My answer");
	EXPECT_NE(reply.plain_text.find("My answer"), std::string::npos);
}

TEST(MimeComposerTest, CreateReply_BodyContainsQuotedOriginal)
{
	Email original = MakeOriginal();
	Email reply = MimeComposer::CreateReply(original, "alice@example.com", "My answer");
	EXPECT_NE(reply.plain_text.find(">"), std::string::npos);
	EXPECT_NE(reply.plain_text.find(original.plain_text), std::string::npos);
}

TEST(MimeComposerTest, CreateReply_DoesNotCopyAttachments)
{
	Email original = MakeOriginal();
	original.AddAttachment("file.txt", "text/plain", "SGVsbG8=");
	Email reply = MimeComposer::CreateReply(original, "alice@example.com", "Body");
	EXPECT_FALSE(reply.HasAttachments());
}


// CreateForward

TEST(MimeComposerTest, CreateForward_RecipientSetCorrectly)
{
	Email original = MakeOriginal();
	Email fwd = MimeComposer::CreateForward(original, "alice@example.com",
												  "dave@example.com", "FYI");
	EXPECT_EQ(fwd.recipient, "dave@example.com");
}

TEST(MimeComposerTest, CreateForward_SenderSetCorrectly)
{
	Email original = MakeOriginal();
	Email fwd = MimeComposer::CreateForward(original, "alice@example.com",
												  "dave@example.com", "FYI");
	EXPECT_EQ(fwd.sender, "alice@example.com");
}

TEST(MimeComposerTest, CreateForward_SubjectHasFwdPrefix)
{
	Email original = MakeOriginal();
	Email fwd = MimeComposer::CreateForward(original, "alice@example.com",
												  "dave@example.com", "FYI");
	EXPECT_EQ(fwd.subject.substr(0, 5), "Fwd: ");
}

TEST(MimeComposerTest, CreateForward_DoesNotDoubleFwdPrefix)
{
	Email original = MakeOriginal();
	original.subject = "Fwd: Question";
	Email fwd = MimeComposer::CreateForward(original, "alice@example.com",
												   "dave@example.com", "FYI");
	EXPECT_EQ(fwd.subject.find("Fwd: Fwd:"), std::string::npos);
}

TEST(MimeComposerTest, CreateForward_CaseInsensitiveFwdPrefix)
{
	Email original = MakeOriginal();
	original.subject = "fwd: Question";
	Email fwd  = MimeComposer::CreateForward(original, "alice@example.com",
												   "dave@example.com", "FYI");
	EXPECT_EQ(fwd.subject.find("Fwd: Fwd:"), std::string::npos);
}

TEST(MimeComposerTest, CreateForward_FwPrefix_AlsoHandled)
{
	Email original = MakeOriginal();
	original.subject = "Fw: Question";
	Email fwd = MimeComposer::CreateForward(original, "alice@example.com",
												   "dave@example.com", "FYI");
	EXPECT_EQ(fwd.subject.find("Fwd: Fw:"), std::string::npos);
}

TEST(MimeComposerTest, CreateForward_CommentTextInBody)
{
	Email original = MakeOriginal();
	Email fwd = MimeComposer::CreateForward(original, "alice@example.com",
												 "dave@example.com", "My comment");
	EXPECT_NE(fwd.plain_text.find("My comment"), std::string::npos);
}

TEST(MimeComposerTest, CreateForward_ForwardedHeaderBlock)
{
	Email original = MakeOriginal();
	Email fwd = MimeComposer::CreateForward(original, "alice@example.com",
												  "dave@example.com", "FYI");
	EXPECT_NE(fwd.plain_text.find("Forwarded message"), std::string::npos);
}

TEST(MimeComposerTest, CreateForward_OriginalTextPresent)
{
	Email original = MakeOriginal();
	Email fwd = MimeComposer::CreateForward(original, "alice@example.com",
												  "dave@example.com", "FYI");
	EXPECT_NE(fwd.plain_text.find(original.plain_text), std::string::npos);
}

TEST(MimeComposerTest, CreateForward_AttachmentsCopied)
{
	Email original = MakeOriginal();
	original.AddAttachment("file.txt", "text/plain", "SGVsbG8=");
	Email fwd = MimeComposer::CreateForward(original, "alice@example.com",
											"dave@example.com", "FYI");
	ASSERT_EQ(fwd.attachments.size(), 1u);
	EXPECT_EQ(fwd.attachments[0].file_name, "file.txt");
}


// Integration: CreateReply/Forward → MimeBuilder

TEST(MimeComposerTest, Integration_ReplyBuildsSuccessfully)
{
	auto logger = MakeLogger();
	Email original = MakeOriginal();
	Email reply = MimeComposer::CreateReply(original, "alice@example.com", "OK");
	std::string mime;
	EXPECT_TRUE(MimeBuilder::BuildEmail(reply, mime, logger));
	EXPECT_NE(mime.find("In-Reply-To:"), std::string::npos);
	EXPECT_NE(mime.find("References:"), std::string::npos);
}

TEST(MimeComposerTest, Integration_ForwardBuildsSuccessfully)
{
	auto logger = MakeLogger();
	Email original = MakeOriginal();
	Email fwd = MimeComposer::CreateForward(original, "alice@example.com",
												   "dave@example.com", "FYI");
	std::string mime;
	EXPECT_TRUE(MimeBuilder::BuildEmail(fwd, mime, logger));
	EXPECT_NE(mime.find("Fwd:"), std::string::npos);
}
