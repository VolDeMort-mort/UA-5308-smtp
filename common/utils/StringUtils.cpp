#include "StringUtils.h"

#include <algorithm>
#include <sstream>
#include <string>

namespace SmtpClient {

std::string StringUtils::Trim(const std::string& str)
{
	size_t start = str.find_first_not_of(" \t\r\n");

	if (start == std::string::npos) return "";

	size_t end = str.find_last_not_of(" \t\r\n");

	return str.substr(start, end - start + 1);
}

size_t StringUtils::FindUtf8SafeEnd(const std::string& text, size_t start, size_t max_bytes)
{
	// all utf8 continuation bytes starts with 10xxxxxx 
	// 10xxxxxx OR 11000000  -> 10000000 true
	// 11xxxxxx OR 11000000  -> 11000000 false
	size_t end = std::min(start + max_bytes, text.length());
	while (end > start && end < text.length()
		   && (static_cast<unsigned char>(text[end]) & 0xC0) == 0x80)
	{
		end--;
	}
	return end;
}

std::string StringUtils::WrapText(const std::string& text, size_t line_length)
{
	// RFC 5322 says that one line can contain max 998 symbols
	// if we have more than that limit we have to cut it into smaller pieces
	// we can make lineLength shorter if needed

	if (line_length > 998) line_length = 998;
	if (line_length < 1) line_length = 1;

	std::string result;
	result.reserve(text.length() + (text.length() / line_length) * 2);

	size_t pos = 0;
	while (pos < text.length())
	{
		if (text[pos] == '\r' || text[pos] == '\n')
		{
			result += "\r\n";
			if (text[pos] == '\r' && pos + 1 < text.length() && text[pos + 1] == '\n')
			{
				pos += 2;
			}
			else
			{
				pos++;
			}
			continue;
		}

		size_t limit = line_length;
		if (pos + limit > text.length())
		{
			limit = text.length() - pos;
		}	

		size_t next_newline = text.find_first_of("\r\n", pos);
		bool newline_in_chunk = (next_newline != std::string::npos && next_newline < pos + limit);

		if (newline_in_chunk)
		{
			result += text.substr(pos, next_newline - pos);
			pos = next_newline;
			continue;
		}

		std::string chunk = text.substr(pos, limit);
		size_t last_space = chunk.find_last_of(" \t");

		size_t split_len = 0;
		bool skip_char = false; 

		if (last_space != std::string::npos && (pos + limit < text.length()))
		{
			split_len = last_space;
			skip_char = true; 
		}
		else
		{
			if (pos + limit == text.length())
			{
				split_len = limit;
			}
			else
			{
				size_t safe_end_idx = FindUtf8SafeEnd(text, pos, limit);

				split_len = safe_end_idx - pos;

				if (split_len == 0 && limit > 0) split_len = limit;
			}
		}

		result += text.substr(pos, split_len);
		result += "\r\n";

		pos += split_len;
		if (skip_char && pos < text.length())
		{
			pos++; 
		}
	}

	return result;
}

} // namespace SmtpClient
