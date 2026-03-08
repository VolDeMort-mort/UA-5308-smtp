#pragma once
#include <string>

namespace SmtpClient {

class MimeDecoder
{
public:
	static std::string DecodeEncodedWord(const std::string& input);
	static std::string DecodeQuotedPrintable(const std::string& input);
};

} // namespace SmtpClient
