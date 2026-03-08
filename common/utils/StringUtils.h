#pragma once
#include <string>

namespace SmtpClient {

class StringUtils
{
public:
	static size_t      FindUtf8SafeEnd(const std::string& text, size_t start, size_t max_bytes);
	static std::string WrapText(const std::string& text, size_t line_length = 78);
};

} // namespace SmtpClient
