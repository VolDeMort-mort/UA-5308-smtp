#pragma once
#include <string>

#include "Logger.h"
#include "Email.h"

namespace SmtpClient {

class MimeParser
{
public:
	static bool ParseEmail(const std::string& raw_mime, Email& out_email,
						   ILogger& logger);

private:
	static void ParseMainHeaders(const std::string& top_headers, Email& out_email, ILogger& logger);
	static void ParseMultipartBody(const std::string& body, const std::string& boundary,
				 				  Email& out_email, ILogger& logger);
	static void ProcessMimePart(const std::string& part_raw, Email& out_email,
				 			   ILogger& logger);
	static void SplitHeadersAndBody(const std::string& raw_block,
									std::string& out_headers, std::string& out_body);
	static std::string GetHeaderValue(const std::string& headers, const std::string& key);
	static std::string ExtractBoundary(const std::string& content_type_header);
	static std::string ExtractCharset(const std::string& content_type_header);
	static std::string ExtractFileName(const std::string& part_headers, ILogger& logger);
};

} // namespace SmtpClient
