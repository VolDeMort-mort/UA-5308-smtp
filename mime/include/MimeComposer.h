#pragma once
#include <string>

#include "Email.h"

namespace SmtpClient {

class MimeComposer
{
public:
	static Email CreateNew(const std::string& from, const std::string& to, const std::string& subject,
						   const std::string& body_text);

	static Email CreateReply(const Email& original, const std::string& from,
							 const std::string& reply_text);

	static Email CreateForward(const Email& original, const std::string& from,
							   const std::string& to, const std::string& comment_text);

private:
	static bool        StartsWithIgnoreCase(const std::string& str, const std::string& prefix);
	static std::string BuildQuotedText(const Email& original);
	static std::string BuildForwardHeader(const Email& original);
};

} // namespace SmtpClient
