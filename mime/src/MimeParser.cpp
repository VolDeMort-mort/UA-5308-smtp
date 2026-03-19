#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>
#include <exception>

#include "Logger.h"
#include "../../common/utils/StringUtils.h" 
#include "../include/MimeParser.h"
#include "../include/MimeDecoder.h"
#include "../../base64/include/Base64Decoder.hpp"
namespace SmtpClient {

namespace {

bool IEquals(const std::string& a, const std::string& b)
{
	if (a.size() != b.size()) return false;
	return std::equal(a.begin(), a.end(), b.begin(),
					  [](unsigned char x, unsigned char y)
					  { return std::tolower(x) == std::tolower(y); });
}

std::string ToLower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(),
				   [](unsigned char c) { return std::tolower(c); });
	return s;
}

} // anonymous namespace

bool MimeParser::ParseEmail(const std::string& raw_mime, Email& out_email,
							ILogger& logger)
{
	if (raw_mime.empty())
	{
		logger.Log(PROD, "Parser error: Raw MIME string is empty.");
		return false;
	}

	std::string top_headers, body;
	SplitHeadersAndBody(raw_mime, top_headers, body);

	ParseMainHeaders(top_headers, out_email, logger);

	std::string content_type = GetHeaderValue(top_headers, "Content-Type:");

	if (ToLower(content_type).find("multipart") != std::string::npos)
	{
		std::string boundary = ExtractBoundary(content_type);

		if (boundary.empty())
		{
			logger.Log(PROD, "Parser error: Multipart detected but no boundary found.");
			out_email.plain_text = body;
			return false;
		}

		logger.Log(DEBUG, "MimeParser: boundary [" + boundary + "]");
		ParseMultipartBody(body, boundary, out_email, logger);
	}
	else
	{
		std::string encoding = ToLower(GetHeaderValue(top_headers, "Content-Transfer-Encoding:"));

		if (encoding.find("quoted-printable") != std::string::npos)
		{
			out_email.plain_text = MimeDecoder::DecodeQuotedPrintable(body);
		}
		else if (encoding.find("base64") != std::string::npos)
		{
			try
			{
				auto decoded = Base64Decoder::DecodeBase64(body);
				out_email.plain_text = std::string(decoded.begin(), decoded.end());
			}
			catch (const std::exception& e)
			{
				logger.Log(PROD, std::string("Base64 decode error: ") + e.what());
				out_email.plain_text = body;
			}
			catch (...)
			{
				logger.Log(PROD, "Base64 decode critical error: Unknown exception!");
				out_email.plain_text = body; 
			}
		}
		else
		{
			out_email.plain_text = body;
		}
	}

	logger.Log(DEBUG, "MimeParser: parsed email from " + out_email.sender);
	return true;
}

void MimeParser::SplitHeadersAndBody(const std::string& raw_block,
									 std::string& out_headers, std::string& out_body)
{
	size_t pos = raw_block.find("\r\n\r\n");
	size_t delimiter_len = 4;

	if (pos == std::string::npos)
	{
		pos = raw_block.find("\n\n");
		delimiter_len = 2;
	}

	if (pos != std::string::npos)
	{
		out_headers = raw_block.substr(0, pos);
		out_body = raw_block.substr(pos + delimiter_len);
	}
	else
	{
		out_headers = "";
		out_body = raw_block;
	}
}

std::string MimeParser::ExtractBoundary(const std::string& content_type_header)
{
	std::string lower_header = ToLower(content_type_header);
	size_t pos = lower_header.find("boundary=");

	if (pos == std::string::npos) return "";

	pos += 9; // length of "boundary="
	if (pos >= content_type_header.length()) return "";

	std::string boundary;
	if (content_type_header[pos] == '"')
	{
		pos++;
		size_t end_pos = content_type_header.find('"', pos);
		if (end_pos != std::string::npos)
			boundary = content_type_header.substr(pos, end_pos - pos);
	}
	else
	{
		size_t end_pos = content_type_header.find_first_of(" ;\r\n", pos);
		if (end_pos != std::string::npos)
			boundary = content_type_header.substr(pos, end_pos - pos);
		else
			boundary = content_type_header.substr(pos);
	}

	return StringUtils::Trim(boundary);
}

void MimeParser::ParseMainHeaders(const std::string& top_headers, Email& out_email, ILogger& logger)
{
	out_email.sender     = MimeDecoder::DecodeEncodedWord(GetHeaderValue(top_headers, "From:"),    &logger);
	out_email.recipient  = MimeDecoder::DecodeEncodedWord(GetHeaderValue(top_headers, "To:"),      &logger);
	out_email.subject    = MimeDecoder::DecodeEncodedWord(GetHeaderValue(top_headers, "Subject:"), &logger);
	out_email.date       = GetHeaderValue(top_headers, "Date:");
	out_email.message_id = GetHeaderValue(top_headers, "Message-ID:");
	out_email.in_reply_to = GetHeaderValue(top_headers, "In-Reply-To:");
	out_email.references = GetHeaderValue(top_headers, "References:");
}

void MimeParser::ParseMultipartBody(const std::string& body, const std::string& boundary,
									Email& out_email, ILogger& logger)
{
	std::string delimiter = "--" + boundary;
	size_t      start_pos = body.find(delimiter);

	while (start_pos != std::string::npos)
	{
		start_pos += delimiter.length();

		if (start_pos < body.length() && body[start_pos] == '\r') start_pos++;
		if (start_pos < body.length() && body[start_pos] == '\n') start_pos++;

		if (start_pos + 1 < body.length() && body.substr(start_pos, 2) == "--")
			break;

		size_t end_pos = body.find(delimiter, start_pos);
		if (end_pos == std::string::npos) break;

		std::string part_raw = body.substr(start_pos, end_pos - start_pos);
		ProcessMimePart(part_raw, out_email, logger);

		start_pos = end_pos;
	}
}

void MimeParser::ProcessMimePart(const std::string& part_raw, Email& out_email,
							 ILogger& logger)
{
	std::string part_headers, part_body;
	SplitHeadersAndBody(part_raw, part_headers, part_body);

	std::string content_type = ToLower(GetHeaderValue(part_headers, "Content-Type:"));
	std::string disposition  = ToLower(GetHeaderValue(part_headers, "Content-Disposition:"));
	std::string encoding     = ToLower(GetHeaderValue(part_headers, "Content-Transfer-Encoding:"));

	if (encoding.find("quoted-printable") != std::string::npos)
		part_body = MimeDecoder::DecodeQuotedPrintable(part_body);

	// Nested multipart — recurse
	if (content_type.find("multipart") != std::string::npos)
	{
		std::string nested_boundary = ExtractBoundary(GetHeaderValue(part_headers, "Content-Type:"));
		if (!nested_boundary.empty())
		{
			logger.Log(DEBUG, "MimeParser: nested multipart (" + content_type + ")");
			ParseMultipartBody(part_body, nested_boundary, out_email, logger);
		}
		return;
	}

	bool is_attachment = (disposition.find("attachment") != std::string::npos)
					  || (content_type.find("name=") != std::string::npos)
					  || (disposition.find("filename=") != std::string::npos);

	if (is_attachment)
	{
		std::string file_name = ExtractFileName(part_headers, logger);

		std::string pure_mime = content_type;
		size_t      semi      = pure_mime.find(';');
		if (semi != std::string::npos) pure_mime = pure_mime.substr(0, semi);

		if (encoding.find("base64") != std::string::npos)
		{
			part_body.erase(std::remove(part_body.begin(), part_body.end(), '\r'), part_body.end());
			part_body.erase(std::remove(part_body.begin(), part_body.end(), '\n'), part_body.end());
		}

		out_email.AddAttachment(file_name, pure_mime, part_body);
		logger.Log(DEBUG, "MimeParser: attachment added: " + file_name);
	}
	else if (content_type.find("text/plain") != std::string::npos)
	{
		if (encoding.find("base64") != std::string::npos)
		{
			part_body.erase(std::remove(part_body.begin(), part_body.end(), '\r'), part_body.end());
			part_body.erase(std::remove(part_body.begin(), part_body.end(), '\n'), part_body.end());
			try
			{
				auto decoded = Base64Decoder::DecodeBase64(part_body);
				out_email.plain_text = std::string(decoded.begin(), decoded.end());
			}
			catch (const std::exception& e)
			{
				logger.Log(PROD, std::string("MimeParser: Base64 decode error (text/plain): ") + e.what());
				out_email.plain_text = part_body;
			}
			catch (...)
			{
				logger.Log(PROD, "MimeParser: Unknown Base64 decode error (text/plain)");
				out_email.plain_text = part_body;
			}
		}
		else
		{
			out_email.plain_text = part_body;
		}
	}
	else if (content_type.find("text/html") != std::string::npos)
	{
		if (encoding.find("base64") != std::string::npos)
		{
			part_body.erase(std::remove(part_body.begin(), part_body.end(), '\r'), part_body.end());
			part_body.erase(std::remove(part_body.begin(), part_body.end(), '\n'), part_body.end());
			try
			{
				auto decoded = Base64Decoder::DecodeBase64(part_body);
				out_email.html_text = std::string(decoded.begin(), decoded.end());
			}
			catch (const std::exception& e)
			{
				logger.Log(PROD, std::string("MimeParser: Base64 decode error (text/html): ") + e.what());
				out_email.html_text = part_body;
			}
			catch (...)
			{
				logger.Log(PROD, "MimeParser: Unknown Base64 decode error (text/html)");
				out_email.html_text = part_body;
			}
		}
		else
		{
			out_email.html_text = part_body;
		}
	}
}

std::string MimeParser::ExtractFileName(const std::string& part_headers, ILogger& logger)
{
	std::string file_name;

	std::string disp    = GetHeaderValue(part_headers, "Content-Disposition:");
	size_t      name_pos = disp.find("filename=");
	if (name_pos != std::string::npos)
	{
		name_pos += 9;
		if (name_pos < disp.length())
		{
			if (disp[name_pos] == '"')
			{
				size_t end = disp.find('"', name_pos + 1);
				if (end != std::string::npos)
					file_name = disp.substr(name_pos + 1, end - name_pos - 1);
			}
			else
			{
				size_t end = disp.find_first_of(" ;\r\n", name_pos);
				file_name  = (end != std::string::npos)
							? disp.substr(name_pos, end - name_pos)
							: disp.substr(name_pos);
			}
		}
	}

	if (file_name.empty())
	{
		std::string ctype = GetHeaderValue(part_headers, "Content-Type:");
		name_pos          = ctype.find("name=");
		if (name_pos != std::string::npos)
		{
			name_pos += 5;
			if (name_pos < ctype.length())
			{
				if (ctype[name_pos] == '"')
				{
					size_t end = ctype.find('"', name_pos + 1);
					if (end != std::string::npos)
						file_name = ctype.substr(name_pos + 1, end - name_pos - 1);
				}
				else
				{
					size_t end = ctype.find_first_of(" ;\r\n", name_pos);
					file_name  = (end != std::string::npos)
								? ctype.substr(name_pos, end - name_pos)
								: ctype.substr(name_pos);
				}
			}
		}
	}

	if (file_name.empty()) file_name = "unnamed_file";
	return MimeDecoder::DecodeEncodedWord(StringUtils::Trim(file_name), &logger);
}

std::string MimeParser::GetHeaderValue(const std::string& headers, const std::string& key)
{
	std::string lower_key = ToLower(key);
	if (!lower_key.empty() && lower_key.back() == ':') lower_key.pop_back();

	std::istringstream stream(headers);
	std::string line;
	std::string result;
	bool found = false;

	while (std::getline(stream, line))
	{
		if (!line.empty() && line.back() == '\r') line.pop_back();
		if (line.empty()) continue;

		if (found && (line[0] == ' ' || line[0] == '\t'))
		{
			std::string appended = StringUtils::Trim(line);
			if (!appended.empty())
			{
				result += " " + appended;
			}
			continue;
		}

		if (found) break;

		size_t colon_pos = line.find(':');
		if (colon_pos == std::string::npos) continue;

		std::string current_key = line.substr(0, colon_pos);
		if (IEquals(current_key, lower_key))
		{
			result = StringUtils::Trim(line.substr(colon_pos + 1));
			found = true;
		}
	}
	return result;
}

} // namespace SmtpClient
