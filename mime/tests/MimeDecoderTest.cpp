#include <gtest/gtest.h>

#include "../include/MimeDecoder.h"

using namespace SmtpClient;

// DecodeEncodedWord — Base64 blocks
TEST(MimeDecoderTest, DecodeEncodedWord_PlainAscii)
{
	EXPECT_EQ(MimeDecoder::DecodeEncodedWord("Hello World"), "Hello World");
}

TEST(MimeDecoderTest, DecodeEncodedWord_EmptyString)
{
	EXPECT_EQ(MimeDecoder::DecodeEncodedWord(""), "");
}

TEST(MimeDecoderTest, DecodeEncodedWord_Base64Cyrillic)
{
	// "Привіт" in Base64: 0KPRgNC40LLRltGC
	std::string encoded = "=?UTF-8?B?0J/RgNC40LLRltGC?=";
	std::string decoded = MimeDecoder::DecodeEncodedWord(encoded);

	EXPECT_EQ(decoded, "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD1\x96\xD1\x82"); // "Привіт"
}

TEST(MimeDecoderTest, DecodeEncodedWord_QEncoding_Underscore)
{
	std::string encoded = "=?UTF-8?Q?Hello_World?=";
	EXPECT_EQ(MimeDecoder::DecodeEncodedWord(encoded), "Hello World");
}

TEST(MimeDecoderTest, DecodeEncodedWord_QEncoding_HexEscape)
{
	std::string encoded = "=?UTF-8?Q?=41=42=43?="; // ABC
	EXPECT_EQ(MimeDecoder::DecodeEncodedWord(encoded), "ABC");
}

TEST(MimeDecoderTest, DecodeEncodedWord_CaseInsensitiveEncoding_B)
{
	// Both ?B? and ?b? should work
	std::string lower  = "=?UTF-8?b?SGVsbG8=?=";
	std::string upper  = "=?UTF-8?B?SGVsbG8=?=";
	EXPECT_EQ(MimeDecoder::DecodeEncodedWord(lower), "Hello");
	EXPECT_EQ(MimeDecoder::DecodeEncodedWord(upper), "Hello");
}

TEST(MimeDecoderTest, DecodeEncodedWord_CaseInsensitiveEncoding_Q)
{
	std::string lower = "=?UTF-8?q?Hello_World?=";
	std::string upper = "=?UTF-8?Q?Hello_World?=";
	EXPECT_EQ(MimeDecoder::DecodeEncodedWord(lower), "Hello World");
	EXPECT_EQ(MimeDecoder::DecodeEncodedWord(upper), "Hello World");
}

TEST(MimeDecoderTest, DecodeEncodedWord_WhitespaceBetweenBlocks_Ignored)
{
	std::string encoded = "=?UTF-8?B?SGVsbG8=?= =?UTF-8?B?V29ybGQ=?=";
	EXPECT_EQ(MimeDecoder::DecodeEncodedWord(encoded), "HelloWorld");
}

TEST(MimeDecoderTest, DecodeEncodedWord_PlainTextBeforeBlock)
{
	std::string encoded = "Prefix =?UTF-8?B?SGVsbG8=?=";
	std::string decoded = MimeDecoder::DecodeEncodedWord(encoded);
	EXPECT_NE(decoded.find("Prefix"), std::string::npos);
	EXPECT_NE(decoded.find("Hello"), std::string::npos);
}

TEST(MimeDecoderTest, DecodeEncodedWord_PlainTextAfterBlock)
{
	std::string encoded = "=?UTF-8?B?SGVsbG8=?= suffix";
	std::string decoded = MimeDecoder::DecodeEncodedWord(encoded);
	EXPECT_NE(decoded.find("Hello"), std::string::npos);
	EXPECT_NE(decoded.find("suffix"), std::string::npos);
}

TEST(MimeDecoderTest, DecodeEncodedWord_MalformedBlock_PassThrough)
{
	std::string malformed = "=?UTF-8?B?no_close";
	std::string decoded   = MimeDecoder::DecodeEncodedWord(malformed);
	EXPECT_EQ(decoded, malformed);
}

TEST(MimeDecoderTest, DecodeEncodedWord_UnknownEncoding_PassThrough)
{
	std::string encoded = "=?UTF-8?X?Hello?=";
	std::string decoded = MimeDecoder::DecodeEncodedWord(encoded);
	EXPECT_EQ(decoded, "Hello");
}

// DecodeQuotedPrintable
TEST(MimeDecoderTest, DecodeQP_PlainAscii)
{
	EXPECT_EQ(MimeDecoder::DecodeQuotedPrintable("Hello World"), "Hello World");
}

TEST(MimeDecoderTest, DecodeQP_EmptyString)
{
	EXPECT_EQ(MimeDecoder::DecodeQuotedPrintable(""), "");
}

TEST(MimeDecoderTest, DecodeQP_HexEscape)
{
	EXPECT_EQ(MimeDecoder::DecodeQuotedPrintable("=41=42=43"), "ABC");
}

TEST(MimeDecoderTest, DecodeQP_SoftLineBreak)
{
	std::string input    = "Hello=\r\nWorld";
	std::string expected = "HelloWorld";
	EXPECT_EQ(MimeDecoder::DecodeQuotedPrintable(input), expected);
}

TEST(MimeDecoderTest, DecodeQP_CyrillicMultibyte)
{
	// "Привіт" = D0 9F D1 80 D0 B8 D0 B2 D1 96 D0 B2 D1 96 D1 82 (actually "Привіт")
	// D0 9F D1 80 D0 B8 D0 B2 D1 96 D1 82
	std::string input    = "=D0=9F=D1=80=D0=B8=D0=B2=D1=96=D1=82";
	std::string expected = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD1\x96\xD1\x82";
	EXPECT_EQ(MimeDecoder::DecodeQuotedPrintable(input), expected);
}

TEST(MimeDecoderTest, DecodeQP_LoneEquals_PassThrough)
{
	EXPECT_EQ(MimeDecoder::DecodeQuotedPrintable("x=yz"), "x=yz");
}

TEST(MimeDecoderTest, DecodeQP_SoftLineBreakLfOnly)
{
	// Some mailers use =\n (LF only) as soft line break
	std::string input    = "Hello=\nWorld";
	std::string expected = "HelloWorld";
	EXPECT_EQ(MimeDecoder::DecodeQuotedPrintable(input), expected);
}

TEST(MimeDecoderTest, DecodeQP_TrailingEqualsSign)
{
	// = at end of string (no following chars) must not crash
	EXPECT_EQ(MimeDecoder::DecodeQuotedPrintable("abc="), "abc=");
}

TEST(MimeDecoderTest, DecodeQP_EqualsFollowedByOneHexChar)
{
	// =A without a second hex digit is a lone equals pass-through
	EXPECT_EQ(MimeDecoder::DecodeQuotedPrintable("=Az"), "=Az");
}

TEST(MimeDecoderTest, DecodeQP_LowercaseHexEscape)
{
	EXPECT_EQ(MimeDecoder::DecodeQuotedPrintable("=61=62=63"), "abc");
}

TEST(MimeDecoderTest, DecodeEncodedWord_MissingFirstQ_PassThrough)
{
	// =?UTF-8 with no following ? is malformed
	std::string s = "=?UTF-8";
	EXPECT_EQ(MimeDecoder::DecodeEncodedWord(s), s);
}

TEST(MimeDecoderTest, DecodeEncodedWord_MissingSecondQ_PassThrough)
{
	// =?UTF-8?Q with no second ?
	std::string s = "=?UTF-8?Q";
	EXPECT_EQ(MimeDecoder::DecodeEncodedWord(s), s);
}

TEST(MimeDecoderTest, DecodeEncodedWord_MissingTerminator_PassThrough)
{
	// =?UTF-8?Q?data (no closing ?=)
	std::string s = "=?UTF-8?Q?no_close";
	EXPECT_EQ(MimeDecoder::DecodeEncodedWord(s), s);
}

TEST(MimeDecoderTest, DecodeEncodedWord_WhitespaceOnlyTailDropped)
{
	// Whitespace between consecutive encoded words should be dropped
	std::string s = "=?UTF-8?B?SGVsbG8=?=   =?UTF-8?B?V29ybGQ=?=";
	EXPECT_EQ(MimeDecoder::DecodeEncodedWord(s), "HelloWorld");
}

TEST(MimeDecoderTest, DecodeEncodedWord_NonWhitespaceTailKept)
{
	// Non-whitespace after encoded word must be preserved
	std::string s = "=?UTF-8?B?SGVsbG8=?= world";
	std::string r = MimeDecoder::DecodeEncodedWord(s);
	EXPECT_NE(r.find("Hello"), std::string::npos);
	EXPECT_NE(r.find("world"), std::string::npos);
}

TEST(MimeDecoderTest, DecodeEncodedWord_PlainTextNoArtifacts)
{
	// Plain text without any QP artifacts or encoded words is returned as-is
	std::string s = "Just a plain subject";
	EXPECT_EQ(MimeDecoder::DecodeEncodedWord(s), s);
}

TEST(MimeDecoderTest, DecodeEncodedWord_QpArtifactsInPlainText)
{
	// If no encoded words but text has QP artifacts it should be QP-decoded
	std::string s = "Hello=20World"; // =20 is a space
	std::string r = MimeDecoder::DecodeEncodedWord(s);
	EXPECT_EQ(r, "Hello World");
}
