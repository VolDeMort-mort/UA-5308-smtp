#pragma once

#include <string>

#include "Logger.h"

namespace SmtpClient {

class MimeDecoder
{
public:
	static std::string DecodeEncodedWord(const std::string& input, ILogger* logger = nullptr);
	static std::string DecodeQuotedPrintable(const std::string& input);
};

} // namespace SmtpClient
