#pragma once
#include <string>
#include <cstddef>

namespace SmtpClient {

constexpr std::size_t MIME_HEADER_CHUNK_SIZE = 45;

class MimeEncoder
{
public:
	static std::string EncodeHeader(const std::string& text,
									const std::string& separator = " ");
	static std::string ToBase64(const std::string& text);
	static bool        NeedsEncoding(const std::string& text);
};

} // namespace SmtpClient
