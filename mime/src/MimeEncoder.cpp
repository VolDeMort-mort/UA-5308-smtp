#include <vector>

#include "Config.h"
#include "MimeEncoder.h"
#include "Base64Encoder.hpp"
#include "StringUtils.h"

namespace SmtpClient {

bool MimeEncoder::NeedsEncoding(const std::string& text)
{
	for (unsigned char c : text)
	{
		if (c < 32 || c > 126) return true;
	}
	return false;
}

std::string MimeEncoder::ToBase64(const std::string& text)
{
	std::vector<uint8_t> bytes(text.begin(), text.end());
	return Base64Encoder::EncodeBase64(bytes);
}

std::string MimeEncoder::EncodeHeader(const std::string& text, const std::string& separator)
{
	if (!NeedsEncoding(text))
		return text;

	std::string result;
	size_t length = text.length();
	int chunk = SmtpClient::Config::Instance().GetMime().header_chunk_size;

	for (size_t i = 0; i < length;)
	{
		size_t end = StringUtils::FindUtf8SafeEnd(text, i, chunk);
		std::string chunk      = text.substr(i, end - i);
		std::string base64_chunk = ToBase64(chunk);

		if (!result.empty())
			result += separator;

		result += "=?UTF-8?B?" + base64_chunk + "?=";
		i = end;
	}
	return result;
}

} // namespace SmtpClient
