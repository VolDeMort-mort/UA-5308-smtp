#pragma once
#include <string>

#include "ILogger.h"
#include "Email.h"

namespace SmtpClient {

class MimeBuilder
{
public:
	static bool BuildEmail(const Email& email_data, std::string& out_mime,
						   ILogger& logger);
	//  requirements from https://www.rfc-editor.org/rfc/rfc2046#section-5.1.1
	//  length 1-70
	//  allowed to use numbers, letters, space, and symbols: ' ( ) + _ , - . / : = ?
	//  Boundary cant end on space
	//  Boundary  must not appear in data
	//  "...result of an algorithm designed to produce boundary
	//  delimiters with a very low probability of already existing in the data..."
	//  "...without having to prescan the data."
	static std::string GenerateBoundary();


private:
	static bool        ValidateEmail(const Email& email_data, ILogger& logger);
	static void        AppendMainHeaders(const Email& email_data, std::string& out);
	static void        AppendBodyContent(const Email& email_data, const std::string& boundary,
										 std::string& out);
	static void        AppendTextPart(const std::string& body, const std::string& boundary,
									  std::string& out);
	static void        AppendHtmlPart(const std::string& html_body, const std::string& boundary,
									  std::string& out);
	static void        AppendAttachments(const Email& email_data, const std::string& boundary,
										 std::string& out);
	static std::string GetMimeType(const std::string& file_name);
	static std::string GenerateMessageId(const std::string& sender);
};

} // namespace SmtpClient
