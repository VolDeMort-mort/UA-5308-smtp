#include "../utils/TimeUtils.h"

#include <gtest/gtest.h>
#include <regex>

using namespace SmtpClient;

TEST(TimeUtilsTest, Rfc2822FormatMatch)
{
	std::string date = TimeUtils::get_current_date();

	// Pattern: Day, DD Mon YYYY HH:MM:SS +ZZZZ
	// Example: Tue, 03 Mar 2026 21:10:00 +0200
	std::regex rfcPattern(R"([A-Z][a-z]{2},\s\d{2}\s[A-Z][a-z]{2}\s\d{4}\s\d{2}:\d{2}:\d{2}\s[+-]\d{4})");

	EXPECT_TRUE(std::regex_match(date, rfcPattern)) << "Date format '" << date << "' does not match RFC 2822";
}

TEST(TimeUtilsTest, CheckValidStaticParts)
{
	std::string date = TimeUtils::get_current_date();

	// Check if it starts with a valid day of week followed by a comma
	std::string day = date.substr(0, 3);
	std::vector<std::string> validDays = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

	bool foundDay = false;
	for (const auto& d : validDays)
	{
		if (day == d) foundDay = true;
	}

	EXPECT_TRUE(foundDay);
	EXPECT_EQ(date[3], ','); // Must have a comma after the day
	EXPECT_EQ(date[4], ' '); // Must have a space after the comma
}

TEST(TimeUtilsTest, TimezoneFormat)
{
	std::string date = TimeUtils::get_current_date();

	// The last 5 characters should be something like +0200
	std::string tz = date.substr(date.length() - 5);

	EXPECT_TRUE(tz[0] == '+' || tz[0] == '-');
	for (int i = 1; i < 5; ++i)
	{
		EXPECT_TRUE(std::isdigit(tz[i]));
	}
}
