#pragma once
#include <string>
#include <vector>

#include "Attachment.h"

namespace SmtpClient {

struct Email
{
	std::string sender;
	std::vector<std::string> to;
	std::vector<std::string> cc;
	std::vector<std::string> bcc;
	std::string subject;
	std::string date;
	std::string charset = "utf-8";

	std::string message_id;
	std::string in_reply_to;
	std::string references;

	std::string plain_text;
	std::string html_text;
	std::string plain_text_encoding;
	std::string html_text_encoding;

	std::string boundary_mixed;
	std::string boundary_alternative;

	std::vector<Attachment> attachments;

	void AddAttachment(const std::string& name, const std::string& mime_type,
					   const std::string& base64_data)
	{
		attachments.emplace_back(name, mime_type, base64_data);
	}

	bool HasAttachments() const { return !attachments.empty(); }

	bool HasHtml() const { return !html_text.empty(); }
};

} // namespace SmtpClient
