#include "../include/MimeBuilder.h"

#include <unordered_map> 
#include <algorithm>     
#include <string>        
#include <chrono>
#include <random>
#include <sstream>

#include "../include/MimeEncoder.h"
#include  "../../common/utils/TimeUtils.h"
#include  "../../common/utils/StringUtils.h"
namespace SmtpClient {

bool MimeBuilder::BuildEmail(const Email& email_data, std::string& out_mime,
							 ILogger& logger)
{
	if (!ValidateEmail(email_data, logger)) return false;

	out_mime.clear();

	std::string mixed_boundary = GenerateBoundary();

	AppendMainHeaders(email_data, out_mime);

	if (email_data.HasAttachments())
	{
		out_mime += "Content-Type: multipart/mixed; boundary=\"" + mixed_boundary + "\"\r\n";
		out_mime += "\r\n";
		AppendBodyContent(email_data, mixed_boundary, out_mime);
		AppendAttachments(email_data, mixed_boundary, out_mime);
		out_mime += "--" + mixed_boundary + "--\r\n";
	}
	
	else
	{
		AppendBodyContent(email_data, mixed_boundary, out_mime);
	}

	logger.Log(DEBUG, "MimeBuilder: built email from " + email_data.sender);
	return true;
}

bool MimeBuilder::ValidateEmail(const Email& email_data, ILogger& logger)
{
	if (StringUtils::Trim(email_data.sender).empty())
	{
		logger.Log(PROD, "MimeBuilder error: sender is empty.");
		return false;
	}
	if (StringUtils::Trim(email_data.recipient).empty())
	{
		logger.Log(PROD, "MimeBuilder error: recipient is empty.");
		return false;
	}
	if (StringUtils::Trim(email_data.plain_text).empty() && email_data.html_text.empty())
	{
		logger.Log(PROD, "MimeBuilder error: body is empty.");
		return false;
	}
	if (StringUtils::Trim(email_data.sender).find('@') == std::string::npos)
	{
		logger.Log(PROD, "MimeBuilder error: invalid sender address.");
		return false;
	}
	if (StringUtils::Trim(email_data.recipient).find('@') == std::string::npos)
	{
		logger.Log(PROD, "MimeBuilder error: invalid recipient address.");
		return false;
	}
	return true;
}

void MimeBuilder::AppendMainHeaders(const Email& email_data, std::string& out)
{
	out += "MIME-Version: 1.0\r\n";
	out += "Date: " + TimeUtils::get_current_date() + "\r\n";

	if (!email_data.subject.empty())
		out += "Subject: " + MimeEncoder::EncodeHeader(email_data.subject, "\r\n\t") + "\r\n";

	out += "From: " + email_data.sender + "\r\n";
	out += "To: " + email_data.recipient + "\r\n";

	std::string msg_id = email_data.message_id;
	if (msg_id.empty())
		msg_id = GenerateMessageId(email_data.sender);
	out += "Message-ID: " + msg_id + "\r\n";

	if (!email_data.in_reply_to.empty())
		out += "In-Reply-To: " + email_data.in_reply_to + "\r\n";
	if (!email_data.references.empty())
		out += "References: " + email_data.references + "\r\n";
}

void MimeBuilder::AppendBodyContent(const Email& email_data, const std::string& boundary,
									std::string& out)
{
	if (email_data.HasHtml())
	{
		std::string alt_boundary = GenerateBoundary();

		if (email_data.HasAttachments())
		{
			out += "--" + boundary + "\r\n";
		}

		out += "Content-Type: multipart/alternative; boundary=\"" + alt_boundary + "\"\r\n";
		out += "\r\n";
		AppendTextPart(email_data.plain_text, alt_boundary, out);
		AppendHtmlPart(email_data.html_text, alt_boundary, out);
		out += "--" + alt_boundary + "--\r\n";

		if (email_data.HasAttachments())
			out += "\r\n";
	}
	else if (email_data.HasAttachments())
	{
		AppendTextPart(email_data.plain_text, boundary, out);
	}
	else
	{
		out += "Content-Type: text/plain; charset=\"utf-8\"\r\n";
		out += "Content-Transfer-Encoding: 8bit\r\n";
		out += "\r\n";
		out += email_data.plain_text + "\r\n";
	}
}

void MimeBuilder::AppendTextPart(const std::string& body, const std::string& boundary,
								 std::string& out)
{
	out += "--" + boundary + "\r\n";
	out += "Content-Type: text/plain; charset=\"utf-8\"\r\n";
	out += "Content-Transfer-Encoding: 8bit\r\n";
	out += "\r\n";
	out += StringUtils::WrapText(body, 998) + "\r\n";
}

void MimeBuilder::AppendHtmlPart(const std::string& html_body, const std::string& boundary,
								 std::string& out)
{
	out += "--" + boundary + "\r\n";
	out += "Content-Type: text/html; charset=\"utf-8\"\r\n";
	out += "Content-Transfer-Encoding: base64\r\n";
	out += "\r\n";
	out += MimeEncoder::ToBase64(html_body) + "\r\n";
}

void MimeBuilder::AppendAttachments(const Email& email_data, const std::string& boundary,
									std::string& out)
{
	for (const Attachment& att : email_data.attachments)
	{
		std::string mime_type   = att.mime_type.empty() ? GetMimeType(att.file_name) : att.mime_type;
		std::string encoded_name = MimeEncoder::EncodeHeader(att.file_name, "\r\n\t");

		out += "--" + boundary + "\r\n";
		out += "Content-Type: " + mime_type + "; name=\"" + encoded_name + "\"\r\n";
		out += "Content-Transfer-Encoding: base64\r\n";
		out += "Content-Disposition: attachment; filename=\"" + encoded_name + "\"\r\n";
		out += "\r\n";
		out += att.base64_data + "\r\n";
	}
}

std::string MimeBuilder::GetMimeType(const std::string& file_name)
{
	size_t dot_pos = file_name.rfind('.');
	if (dot_pos == std::string::npos) return "application/octet-stream";

	std::string ext = file_name.substr(dot_pos + 1);
	std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

	static const std::unordered_map<std::string, std::string> mime_map = {
		{"pdf", "application/pdf"},
		{"jpg", "image/jpeg"},
		{"jpeg", "image/jpeg"},
		{"png", "image/png"},
		{"gif", "image/gif"},
		{"txt", "text/plain"},
		{"html", "text/html"},
		{"htm", "text/html"},
		{"xml", "application/xml"},
		{"zip", "application/zip"},
		{"doc", "application/msword"},
		{"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
		{"xls", "application/vnd.ms-excel"},
		{"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
		{"ppt", "application/vnd.ms-powerpoint"},
		{"pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
		{"json", "application/json"},
		{"csv", "text/csv"}};

	auto it = mime_map.find(ext);
	if (it != mime_map.end())
	{
		return it->second;
	}

	return "application/octet-stream";
}

std::string MimeBuilder::GenerateBoundary()
{
	std::random_device               rd;
	std::mt19937                     gen(rd());
	std::uniform_int_distribution<>  dist(0, 15);

	const char* hex_chars = "0123456789abcdef";
	std::string boundary  = "----=_Part_";
	for (int i = 0; i < 24; ++i)
		boundary += hex_chars[dist(gen)];

	return boundary;
}

std::string MimeBuilder::GenerateMessageId(const std::string& sender)
{
	std::random_device               rd;
	std::mt19937                     gen(rd());
	std::uniform_int_distribution<>  dist(0, 15);

	auto now = std::chrono::system_clock::now().time_since_epoch();
	auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

	const char* hex_chars = "0123456789abcdef";
	std::string unique_id;
	for (int i = 0; i < 32; ++i)
		unique_id += hex_chars[dist(gen)];

	std::string domain = "localhost";
	size_t      at_pos = sender.find('@');
	if (at_pos != std::string::npos)
		domain = sender.substr(at_pos + 1);

	size_t angle_end = domain.find('>');
	if (angle_end != std::string::npos) domain = domain.substr(0, angle_end);

	return "<" + std::to_string(millis) + "." + unique_id + "@" + domain + ">";
	
}

} // namespace SmtpClient
