#include <gtest/gtest.h>

#include "../include/MimeEncoder.h"
#include "../include/MimeDecoder.h"

using namespace SmtpClient;

TEST(MimeEncoderTest, NeedsEncoding_AsciiReturnsFalse)
{
	EXPECT_FALSE(MimeEncoder::NeedsEncoding("Hello World"));
	EXPECT_FALSE(MimeEncoder::NeedsEncoding("alice@example.com"));
	EXPECT_FALSE(MimeEncoder::NeedsEncoding("Test 123!"));
}

TEST(MimeEncoderTest, NeedsEncoding_CyrillicReturnsTrue)
{
	EXPECT_TRUE(MimeEncoder::NeedsEncoding("\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD1\x96\xD1\x82"));
}

TEST(MimeEncoderTest, NeedsEncoding_ControlCharReturnsTrue)
{
	std::string str = "Hello";
	str += static_cast<char>(0x01);
	EXPECT_TRUE(MimeEncoder::NeedsEncoding(str));
}

TEST(MimeEncoderTest, NeedsEncoding_EmptyStringReturnsFalse)
{
	EXPECT_FALSE(MimeEncoder::NeedsEncoding(""));
}

// EncodeHeader
TEST(MimeEncoderTest, EncodeHeader_AsciiPassthrough)
{
	std::string result = MimeEncoder::EncodeHeader("Hello");
	EXPECT_EQ(result, "Hello");
}

TEST(MimeEncoderTest, EncodeHeader_ContainsRfc2047Markers)
{
	std::string encoded = MimeEncoder::EncodeHeader("\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD1\x96\xD1\x82");
	EXPECT_NE(encoded.find("=?UTF-8?B?"), std::string::npos);
	EXPECT_NE(encoded.find("?="), std::string::npos);
}

TEST(MimeEncoderTest, EncodeHeader_SingleBlockForShortText)
{
	std::string encoded = MimeEncoder::EncodeHeader("\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD1\x96\xD1\x82");
	size_t first = encoded.find("=?UTF-8?B?");
	size_t last = encoded.rfind("=?UTF-8?B?");
	EXPECT_EQ(first, last) << "Short text should produce exactly one encoded block";
}

TEST(MimeEncoderTest, EncodeHeader_MultipleBlocksForLongText)
{
	std::string cyrillic;
	for (int i = 0; i < 24; ++i) cyrillic += "\xD0\x90"; // "А" x 24 = 48 bytes
	std::string encoded = MimeEncoder::EncodeHeader(cyrillic);

	size_t pos = 0;
	int count = 0;
	while ((pos = encoded.find("=?UTF-8?B?", pos)) != std::string::npos)
	{
		count++;
		pos += 10;
	}
	EXPECT_GE(count, 2);
}

TEST(MimeEncoderTest, EncodeHeader_CustomSeparator)
{
	std::string cyrillic;
	for (int i = 0; i < 24; ++i) cyrillic += "\xD0\x90";
	std::string encoded = MimeEncoder::EncodeHeader(cyrillic, "\r\n\t");
	EXPECT_NE(encoded.find("\r\n\t"), std::string::npos);
}

TEST(MimeEncoderTest, EncodeHeader_NoRawCrLfInOutput)
{
	std::string cyrillic;
	for (int i = 0; i < 24; ++i) cyrillic += "\xD0\x90";

	std::string encoded = MimeEncoder::EncodeHeader(cyrillic, " ");
	EXPECT_EQ(encoded.find("\r\n"), std::string::npos);
}

TEST(MimeEncoderTest, RoundTrip_Cyrillic)
{
	std::string original = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD1\x96\xD1\x82"; // Привіт
	std::string encoded  = MimeEncoder::EncodeHeader(original);
	std::string decoded  = MimeDecoder::DecodeEncodedWord(encoded);
	EXPECT_EQ(decoded, original);
}

TEST(MimeEncoderTest, RoundTrip_LongCyrillic45ByteChunk)
{
	std::string original;
	for (int i = 0; i < 22; ++i) original += "\xD0\x90"; // exactly 44 bytes (fits in one chunk)
	std::string encoded = MimeEncoder::EncodeHeader(original);
	std::string decoded = MimeDecoder::DecodeEncodedWord(encoded);
	EXPECT_EQ(decoded, original);
}

TEST(MimeEncoderTest, RoundTrip_LongCyrillic46ByteChunk)
{
	std::string original;
	for (int i = 0; i < 24; ++i) original += "\xD0\x90"; // 48 bytes (forces 2 chunks)
	std::string encoded = MimeEncoder::EncodeHeader(original);
	std::string decoded = MimeDecoder::DecodeEncodedWord(encoded);
	EXPECT_EQ(decoded, original);
}
