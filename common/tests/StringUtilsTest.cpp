#include <gtest/gtest.h>

#include "../utils/StringUtils.h"

using namespace SmtpClient;

TEST(StringUtilsTest, SimpleWrap)
{
	std::string input = "Hello World From C++";
	// we try to split by space
	std::string expected = "Hello World\r\nFrom C++\r\n";

	EXPECT_EQ(StringUtils::WrapText(input, 12), expected);
}

TEST(StringUtilsTest, HardWrapLongWord)
{
	// no spaces, so split at the limit
	std::string input = "ABCDEFGHIJ";
	std::string expected = "ABCDE\r\nFGHIJ\r\n";

	EXPECT_EQ(StringUtils::WrapText(input, 5), expected);
}

TEST(StringUtilsTest, PreservesExistingNewlines)
{
	std::string input = "Line1\r\nLine2";
	// Existing \r\n should be kept
	// logic only ensures CRLF at the end
	std::string expected = "Line1\r\nLine2\r\n";

	EXPECT_EQ(StringUtils::WrapText(input, 50), expected);
}

TEST(StringUtilsTest, Utf8SafetyCyrillic)
{
	// Limit is 3 bytes. 'П' (2 bytes) fits. 'р' (2 bytes) would exceed limit (2+2=4).
	// Ensure 'р' is not split in half but moved to the next line.
	std::string input = "Привіт";

	std::string result = StringUtils::WrapText(input, 3);
	std::string expected = "П\r\nр\r\nи\r\nв\r\nі\r\nт\r\n";

	EXPECT_EQ(result, expected);
}

TEST(StringUtilsTest, Utf8Emoji)
{
	// Limit is 6 bytes. "Hello" takes 5. Only 1 byte left for the 4-byte Emoji.
	// The Emoji must be moved entirely to the next line
	std::string input = "Hello🌍";

	std::string result = StringUtils::WrapText(input, 6);
	std::string expected = "Hello\r\n🌍\r\n";

	EXPECT_EQ(result, expected);
}

TEST(StringUtilsTest, RfcHardLimit998)
{
	// Even if we request 2000 chars, the function must enforce the 998 limit per RFC
	std::string input(1000, 'a');

	std::string result = StringUtils::WrapText(input, 2000);

	size_t firstNewline = result.find("\r\n");

	EXPECT_EQ(firstNewline, 998); 
}

TEST(StringUtilsTest, Utf8SafeEndAtStringBounds)
{
	// "При" in UTF-8: \xD0\x9F \xD0\xBE \xD0\xB8 (6 bytes total)
	std::string input = "При";

	// Scenario A: maxBytes is exactly the length of the string
	// The old code might have checked text[3], which is out of bounds.
	// The new code should safely return 6.
	size_t end = StringUtils::FindUtf8SafeEnd(input, 0, 6);
	EXPECT_EQ(end, 6);

	// Scenario B: maxBytes is larger than the string length
	// Should safely return 6 without crashing.
	end = StringUtils::FindUtf8SafeEnd(input, 0, 10);
	EXPECT_EQ(end, 6);

	// Scenario C: Cutting in the middle of a 4-byte character at the very end
	// Emoji 🌍 is \xF0\x9F\x8C\x8D (4 bytes)
	std::string emojiInput = u8"🌍";

	// If we try to take 3 bytes, it should back off to 0, because
	// we can't take a partial Emoji.
	end = StringUtils::FindUtf8SafeEnd(emojiInput, 0, 3);
	EXPECT_EQ(end, 0);
}
