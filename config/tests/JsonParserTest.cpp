#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include <unistd.h>
#include <string>

#include "JsonParser.h"

using SmtpClient::JsonParser;

TEST(JsonParserTest, ParseEmptyObject)
{
	auto result = JsonParser::Parse("{}");
	EXPECT_TRUE(result.empty());
}

TEST(JsonParserTest, ParseSimpleStringValue)
{
	auto result = JsonParser::Parse(R"({"key": "value"})");
	ASSERT_EQ(result.count("key"), 1);
	EXPECT_EQ(result.at("key"), "value");
}

TEST(JsonParserTest, ParseSimpleIntValue)
{
	auto result = JsonParser::Parse(R"({"port": 25000})");
	ASSERT_EQ(result.count("port"), 1);
	EXPECT_EQ(result.at("port"), "25000");
}

TEST(JsonParserTest, ParseNestedObject)
{
	auto result = JsonParser::Parse(R"({"server": {"port": 8080, "domain": "example.com"}})");
	ASSERT_EQ(result.count("server.port"), 1);
	EXPECT_EQ(result.at("server.port"), "8080");
	ASSERT_EQ(result.count("server.domain"), 1);
	EXPECT_EQ(result.at("server.domain"), "example.com");
}

TEST(JsonParserTest, ParseBooleanValues)
{
	auto result = JsonParser::Parse(R"({"enabled": true, "disabled": false})");
	EXPECT_EQ(result.at("enabled"), "true");
	EXPECT_EQ(result.at("disabled"), "false");
}

TEST(JsonParserTest, ParseNullValue)
{
	auto result = JsonParser::Parse(R"({"value": null})");
	EXPECT_EQ(result.at("value"), "null");
}

TEST(JsonParserTest, ParseNegativeNumber)
{
	auto result = JsonParser::Parse(R"({"offset": -100})");
	EXPECT_EQ(result.at("offset"), "-100");
}

TEST(JsonParserTest, ParseFloatingPointNumber)
{
	auto result = JsonParser::Parse(R"({"ratio": 3.14})");
	EXPECT_EQ(result.at("ratio"), "3.14");
}

TEST(JsonParserTest, ParseEscapedStringCharacters)
{
	auto result = JsonParser::Parse(R"({"path": "C:\\Users\\test"})");
	EXPECT_EQ(result.at("path"), "C:\\Users\\test");
}

TEST(JsonParserTest, ParseMultipleTopLevelKeys)
{
	auto result = JsonParser::Parse(R"({"a": "1", "b": "2", "c": "3"})");
	EXPECT_EQ(result.size(), 3);
	EXPECT_EQ(result.at("a"), "1");
	EXPECT_EQ(result.at("b"), "2");
	EXPECT_EQ(result.at("c"), "3");
}

TEST(JsonParserTest, ParseDeeplyNestedObject)
{
	auto result = JsonParser::Parse(R"({"level1": {"level2": {"key": "deep"}}})");
	ASSERT_EQ(result.count("level1.level2.key"), 1);
	EXPECT_EQ(result.at("level1.level2.key"), "deep");
}

TEST(JsonParserTest, ParseWhitespaceVariations)
{
	std::string json = "  {  \"key\"  :  \"value\"  }  ";
	auto result = JsonParser::Parse(json);
	EXPECT_EQ(result.at("key"), "value");
}

TEST(JsonParserTest, ParseFileReturnsEmptyForMissingFile)
{
	auto result = JsonParser::ParseFile("nonexistent_file_12345.json");
	EXPECT_TRUE(result.empty());
}

TEST(JsonParserTest, ParseFileReadsValidJson)
{
	const std::string path = "/tmp/test_config_parser_" + std::to_string(getpid()) + ".json";
	{
		std::ofstream f(path);
		f << R"({"server": {"port": 9999}})";
	}

	auto result = JsonParser::ParseFile(path);
	EXPECT_EQ(result.at("server.port"), "9999");

	std::remove(path.c_str());
}

TEST(JsonParserTest, ParseEmptyStringInput)
{
	auto result = JsonParser::Parse("");
	EXPECT_TRUE(result.empty());
}

TEST(JsonParserTest, ParseMultipleSections)
{
	std::string json = R"({
		"server": {"port": 25000, "domain": "localhost"},
		"logging": {"log_level": "DEBUG", "max_file_size_mb": 10}
	})";
	auto result = JsonParser::Parse(json);
	EXPECT_EQ(result.at("server.port"), "25000");
	EXPECT_EQ(result.at("server.domain"), "localhost");
	EXPECT_EQ(result.at("logging.log_level"), "DEBUG");
	EXPECT_EQ(result.at("logging.max_file_size_mb"), "10");
}

// ─────────────────────────────────────────────────────────────────────────────
// ParseLiteral: unknown token, partial/edge inputs
// ─────────────────────────────────────────────────────────────────────────────

TEST(JsonParserTest, ParseLiteralUnknownToken)
{
	// An unknown token (not true/false/null) should be captured up to delimiter
	auto result = JsonParser::Parse(R"({"x": unknown})");
	EXPECT_EQ(result.at("x"), "unknown");
}

TEST(JsonParserTest, ParseLiteralUnknownTokenBeforeComma)
{
	auto result = JsonParser::Parse(R"({"a": foo, "b": "bar"})");
	EXPECT_EQ(result.at("a"), "foo");
	EXPECT_EQ(result.at("b"), "bar");
}

TEST(JsonParserTest, ParseEscapeSequenceNewline)
{
	auto r = JsonParser::Parse(R"({"a": "line1\nline2"})");
	EXPECT_EQ(r.at("a"), "line1\nline2");
}

TEST(JsonParserTest, ParseEscapeSequenceCarriageReturn)
{
	auto r = JsonParser::Parse(R"({"a": "a\rb"})");
	EXPECT_EQ(r.at("a"), "a\rb");
}

TEST(JsonParserTest, ParseEscapeSequenceTab)
{
	auto r = JsonParser::Parse(R"({"a": "a\tb"})");
	EXPECT_EQ(r.at("a"), "a\tb");
}

TEST(JsonParserTest, ParseEscapeSequenceForwardSlash)
{
	auto r = JsonParser::Parse(R"({"a": "a\/b"})");
	EXPECT_EQ(r.at("a"), "a/b");
}

TEST(JsonParserTest, ParseNumberScientificNotation)
{
	auto result = JsonParser::Parse(R"({"n": 1.5e10})");
	EXPECT_EQ(result.at("n"), "1.5e10");
}

TEST(JsonParserTest, ParseNumberWithPositiveExponent)
{
	auto result = JsonParser::Parse(R"({"n": 2E+3})");
	EXPECT_EQ(result.at("n"), "2E+3");
}

TEST(JsonParserTest, ParseInputWithNoOpeningBrace)
{
	// Input that doesn't start with { returns empty map
	auto result = JsonParser::Parse(R"("just a string")");
	EXPECT_TRUE(result.empty());
}

TEST(JsonParserTest, ParseEmptyNestedObject)
{
	auto result = JsonParser::Parse(R"({"inner": {}})");
	// Empty nested object contributes no keys
	EXPECT_TRUE(result.find("inner") == result.end());
}
