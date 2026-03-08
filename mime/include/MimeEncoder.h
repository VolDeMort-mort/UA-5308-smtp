#pragma once
#include <string>

namespace SmtpClient {

class MimeEncoder
{
public:
	static std::string EncodeHeader(const std::string& text,
									const std::string& separator = " ");
	static std::string ToBase64(const std::string& text);
	static bool        NeedsEncoding(const std::string& text);
};

} // namespace SmtpClient
