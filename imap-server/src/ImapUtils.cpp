#include "ImapUtils.hpp"

#include <algorithm>
#include <fstream>
#include <numeric>
#include <sstream>
#include <unordered_map>

#include "Entity/Recipient.h"
#include "MimeBuilder.h"
#include "MimePart.h"
#include "Repository/MessageRepository.h"
#include "StringUtils.h"

namespace IMAP_UTILS
{

std::string ToUpper(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::toupper(c); });
	return str;
}

std::vector<std::string> SplitArgs(const std::string& str)
{
	std::vector<std::string> args;
	std::istringstream iss(str);
	std::string arg;
	while (iss >> arg)
	{
		args.push_back(arg);
	}
	return args;
}

std::string JoinArgs(const std::vector<std::string>& args)
{
	if (args.empty()) return "";
	std::string result;
	for (size_t i = 0; i < args.size(); ++i)
	{
		result += args[i];
		if (i < args.size() - 1) result += ", ";
	}
	return result;
}

ImapCommandType StringToCommandType(const std::string& cmd)
{
	static const std::unordered_map<std::string, ImapCommandType> commandMap = {
		{"LOGIN", ImapCommandType::Login},
		{"LOGOUT", ImapCommandType::Logout},
		{"CAPABILITY", ImapCommandType::Capability},
		{"NOOP", ImapCommandType::Noop},
		{"SELECT", ImapCommandType::Select},
		{"LIST", ImapCommandType::List},
		{"LSUB", ImapCommandType::Lsub},
		{"STATUS", ImapCommandType::Status},
		{"FETCH", ImapCommandType::Fetch},
		{"STORE", ImapCommandType::Store},
		{"CREATE", ImapCommandType::Create},
		{"DELETE", ImapCommandType::Delete},
		{"RENAME", ImapCommandType::Rename},
		{"COPY", ImapCommandType::Copy},
		{"EXPUNGE", ImapCommandType::Expunge},
		{"UID", ImapCommandType::UidFetch},
		{"SUBSCRIBE", ImapCommandType::Subscribe},
		{"UNSUBSCRIBE", ImapCommandType::Unsubscribe},
		{"CLOSE", ImapCommandType::Close},
		{"CHECK", ImapCommandType::Check},
		{"STARTTLS", ImapCommandType::StartTLS}
	};

	auto it = commandMap.find(IMAP_UTILS::ToUpper(cmd));
	if (it != commandMap.end())
	{
		return it->second;
	}
	return ImapCommandType::Unknown;
}

std::string CommandTypeToString(const ImapCommandType& type)
{
	static const std::unordered_map<ImapCommandType, std::string> commandMap = {
		{ImapCommandType::Login, "LOGIN"},
		{ImapCommandType::Logout, "LOGOUT"},
		{ImapCommandType::Capability, "CAPABILITY"},
		{ImapCommandType::Noop, "NOOP"},
		{ImapCommandType::Select, "SELECT"},
		{ImapCommandType::List, "LIST"},
		{ImapCommandType::Lsub, "LSUB"},
		{ImapCommandType::Status, "STATUS"},
		{ImapCommandType::Fetch, "FETCH"},
		{ImapCommandType::Store, "STORE"},
		{ImapCommandType::Create, "CREATE"},
		{ImapCommandType::Delete, "DELETE"},
		{ImapCommandType::Rename, "RENAME"},
		{ImapCommandType::Copy, "COPY"},
		{ImapCommandType::Expunge, "EXPUNGE"},
		{ImapCommandType::UidFetch, "UID FETCH"},
		{ImapCommandType::UidStore, "UID STORE"},
		{ImapCommandType::UidCopy, "UID COPY"},
		{ImapCommandType::Subscribe, "SUBSCRIBE"},
		{ImapCommandType::Unsubscribe, "UNSUBSCRIBE"},
		{ImapCommandType::Close, "CLOSE"},
		{ImapCommandType::Check, "CHECK"},
		{ImapCommandType::StartTLS, "STARTTLS"}
	};

	auto it = commandMap.find(type);
	if (it != commandMap.end())
	{
		return it->second;
	}
	return "UNKNOWN";
}

std::string TrimParentheses(const std::string& str)
{
	if (str.size() >= 2 && str.front() == '(' && str.back() == ')')
	{
		return str.substr(1, str.size() - 2);
	}
	return str;
}

std::vector<std::string> Split(const std::string& str, char delim)
{
	std::vector<std::string> result;
	std::stringstream ss(str);
	std::string token;

	while (std::getline(ss, token, delim))
	{
		result.push_back(token);
	}

	return result;
}

std::vector<int64_t> ParseSequenceSet(const std::string& sequenceSet, int64_t maxValue)
{
	std::vector<int64_t> res;

	auto parts = Split(sequenceSet, ',');

	for (const auto& part : parts)
	{
		size_t colon_pos = part.find(':');

		if (colon_pos != std::string::npos)
		{
			std::string firstPart = part.substr(0, colon_pos);
			std::string secondPart = part.substr(colon_pos + 1);

			int64_t first = (firstPart == "*") ? maxValue : std::stoll(firstPart);
			int64_t second = (secondPart == "*") ? maxValue : std::stoll(secondPart);

			if (first > second)
			{
				std::swap(first, second);
			}

			for (int64_t i = first; i <= second; ++i)
			{
				res.push_back(i);
			}
		}
		else
		{
			if (part == "*")
			{
				res.push_back(maxValue);
			}
			else
			{
				res.push_back(std::stoll(part));
			}
		}
	}

	std::sort(res.begin(), res.end());
	res.erase(std::unique(res.begin(), res.end()), res.end());

	return res;
}

void SortMessagesByTimeDescending(std::vector<Message>& messages)
{
	std::sort(messages.begin(), messages.end(),
			  [](const Message& a, const Message& b) { return a.internal_date > b.internal_date; });
}

std::string DateToEmlDate(const std::string& str)
{
	if (str.empty()) throw std::runtime_error("DateToEmlDate: empty date string");

	static const std::string months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
										 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	static const std::string days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

	std::stringstream ss(str);
	int year, month, day, hour, minute, second;
	char dash1, dash2, colon1, colon2;

	if (!(ss >> year >> dash1 >> month >> dash2 >> day >> hour >> colon1 >> minute >> colon2 >> second))
	{
		throw std::runtime_error("DateToEmlDate: failed to parse date string '" + str + "'");
	}

	if (dash1 != '-' || dash2 != '-' || colon1 != ':' || colon2 != ':')
	{
		throw std::runtime_error("DateToEmlDate: invalid date format in '" + str + "'");
	}

	if (year < 1900 || month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 || minute < 0 ||
		minute > 59 || second < 0 || second > 59)
	{
		throw std::runtime_error("DateToEmlDate: invalid date values in '" + str + "'");
	}

	int adjustedYear = year;
	int adjustedMonth = month;
	if (adjustedMonth < 3)
	{
		adjustedMonth += 12;
		adjustedYear--;
	}
	int dow = (day + (13 * (adjustedMonth + 1)) / 5 + adjustedYear + adjustedYear / 4 - adjustedYear / 100 +
			   adjustedYear / 400) %
			  7;
	dow = (dow + 6) % 7;

	return days[dow] + ", " + (day < 10 ? "0" : "") + std::to_string(day) + " " + months[month - 1] + " " +
		   std::to_string(year) + " " + (hour < 10 ? "0" : "") + std::to_string(hour) + ":" + (minute < 10 ? "0" : "") +
		   std::to_string(minute) + ":" + (second < 10 ? "0" : "") + std::to_string(second) + " +0000";
}

std::string DateToIMAPInternal(const std::string& str)
{
	if (str.empty()) throw std::runtime_error("DateToIMAPInternal: empty date string");

	static const std::string months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
										 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	std::stringstream ss(str);
	int year, month, day, hour, minute, second;
	char dash1, dash2, colon1, colon2;

	if (!(ss >> year >> dash1 >> month >> dash2 >> day >> hour >> colon1 >> minute >> colon2 >> second))
	{
		throw std::runtime_error("DateToIMAPInternal: failed to parse date string '" + str + "'");
	}

	if (dash1 != '-' || dash2 != '-' || colon1 != ':' || colon2 != ':')
	{
		throw std::runtime_error("DateToIMAPInternal: invalid date format in '" + str + "'");
	}

	if (year < 1900 || month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 || minute < 0 ||
		minute > 59 || second < 0 || second > 59)
	{
		throw std::runtime_error("DateToIMAPInternal: invalid date values in '" + str + "'");
	}

	return (day < 10 ? "0" : "") + std::to_string(day) + "-" + months[month - 1] + "-" + std::to_string(year) + " " +
		   (hour < 10 ? "0" : "") + std::to_string(hour) + ":" + (minute < 10 ? "0" : "") + std::to_string(minute) +
		   ":" + (second < 10 ? "0" : "") + std::to_string(second) + " +0000";
}

std::optional<SmtpClient::Email> GetParsedEmail(const Message& msg, ILogger& logger)
{
	std::ifstream file(msg.raw_file_path);
	if (!file.is_open())
	{
		return std::nullopt;
	}
	std::string raw_mime{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

	SmtpClient::Email email;
	if (!SmtpClient::MimeParser::ParseEmail(raw_mime, email, logger))
	{
		return std::nullopt;
	}

	return email;
}

std::optional<SmtpClient::MimePart> GetParsedMimePart(const Message& msg, ILogger& logger)
{
	std::ifstream file(msg.raw_file_path);
	if (!file.is_open())
	{
		return std::nullopt;
	}
	std::string raw_mime{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

	SmtpClient::MimePart root;
	if (!SmtpClient::MimeParser::ParseStructure(raw_mime, root, logger))
	{
		return std::nullopt;
	}

	return root;
}

// Format: ((name route mailbox host))
std::string FormatAddress(const std::string& raw_email)
{
	if (raw_email.empty() || raw_email == "NIL") return "NIL";

	std::string personal_name = "NIL";
	std::string email_part = raw_email;

	auto bracket_start = raw_email.find('<');
	auto bracket_end = raw_email.find('>');

	if (bracket_start != std::string::npos && bracket_end != std::string::npos && bracket_end > bracket_start)
	{
		if (bracket_start > 0)
		{
			std::string name = raw_email.substr(0, bracket_start);
			auto name_end = name.find_last_not_of(" \t");
			if (name_end != std::string::npos)
			{
				name = name.substr(0, name_end + 1);
				if (name.front() == '"' && name.back() == '"' && name.length() > 1)
				{
					name = name.substr(1, name.length() - 2);
				}
				personal_name = "\"" + name + "\"";
			}
		}
		email_part = raw_email.substr(bracket_start + 1, bracket_end - bracket_start - 1);
	}

	auto at_pos = email_part.find('@');
	std::string mailbox = (at_pos != std::string::npos) ? email_part.substr(0, at_pos) : email_part;
	std::string host = (at_pos != std::string::npos) ? email_part.substr(at_pos + 1) : "unknown";

	return "(" + personal_name + " NIL \"" + mailbox + "\" \"" + host + "\")";
}

std::string BuildEnvelope(const Message& msg, const std::optional<SmtpClient::Email>& email_opt,
						  MessageRepository& messRepo)
{
	std::string sender_email = email_opt ? email_opt->sender : "";
	std::string subject = email_opt ? email_opt->subject : msg.subject.value_or("");
	std::string message_id = email_opt ? email_opt->message_id : ("<" + std::to_string(msg.id.value()) + "@test.com>");
	std::string in_reply_to = email_opt ? email_opt->in_reply_to : msg.in_reply_to.value_or("NIL");
	std::string date = email_opt ? email_opt->date : msg.internal_date;

	auto all_receipients = messRepo.findRecipientsByMessage(msg.id.value());
	std::vector<Recipient> rec_to, rec_cc, rec_bcc;
	for (auto& rec : all_receipients)
	{
		if (rec.type == RecipientType::To)
		{
			rec_to.push_back(rec);
		}
		else if (rec.type == RecipientType::Cc)
		{
			rec_cc.push_back(rec);
		}
		else if (rec.type == RecipientType::Bcc)
		{
			rec_bcc.push_back(rec);
		}
	}

	auto get_list_of_receipients = [](std::vector<Recipient>& recs) -> std::string
	{
		if (recs.empty())
		{
			return "NIL ";
		}

		std::string list = "(";
		for (auto& rec : recs)
		{
			list += FormatAddress(rec.address) + " ";
		}
		list.back() = ')';

		return list;
	};

	std::string env = "(";
	env += "\"" + (email_opt ? date : DateToEmlDate(date)) + "\" ";
	env += "\"" + subject + "\" ";
	env += FormatAddress(sender_email) + " ";
	env += FormatAddress(sender_email) + " ";
	env += FormatAddress(sender_email) + " ";
	env += get_list_of_receipients(rec_to);
	env += get_list_of_receipients(rec_cc);
	env += get_list_of_receipients(rec_bcc);
	env += (!in_reply_to.empty() && in_reply_to != "NIL") ? ("<" + in_reply_to + "> ") : "NIL ";
	env += "\"" + message_id + "\"";
	env += ")";
	return env;
}

std::string BuildBodystructure(const Message& msg, const std::optional<SmtpClient::Email>& email_opt,
							   const std::optional<SmtpClient::MimePart>& mime_part)
{
	if (mime_part)
	{
		return BuildBodystructureFromMimePart(*mime_part);
	}

	if (!email_opt)
	{
		return "(\"text\" \"plain\" (\"charset\" \" utf-8 \") NIL NIL \"7bit\" " + std::to_string(msg.size_bytes) +
			   " 0)";
	}

	bool has_html = email_opt->HasHtml();
	bool has_attach = email_opt->HasAttachments();

	auto count_lines = [](const std::string& str) { return std::count(str.begin(), str.end(), '\n'); };

	std::string charset_params = "(\"charset\" \"" + email_opt->charset + "\")";
	std::string plain_part = "(\"text\" \"plain\" " + charset_params + " NIL NIL \"" + email_opt->plain_text_encoding +
							 "\" " + std::to_string(email_opt->plain_text.size()) + " " +
							 std::to_string(count_lines(email_opt->plain_text)) + ")";

	std::string html_part = "";
	if (has_html)
	{
		html_part = "(\"text\" \"html\" (\"charset\" \"" + email_opt->charset + "\") NIL NIL \"" +
					email_opt->html_text_encoding + "\" " + std::to_string(email_opt->html_text.size()) + " " +
					std::to_string(count_lines(email_opt->html_text)) + ")";
	}

	std::string text_block;
	if (has_html)
	{
		std::string alt_bound = email_opt->boundary_alternative.empty() ? SmtpClient::MimeBuilder::GenerateBoundary()
																		: email_opt->boundary_alternative;
		text_block = "(" + plain_part + html_part + " \"alternative\" (\"boundary\" \"" + alt_bound + "\") NIL NIL)";
	}
	else
	{
		text_block = plain_part;
	}

	if (!has_attach)
	{
		return text_block;
	}

	std::string mix_bound =
		email_opt->boundary_mixed.empty() ? SmtpClient::MimeBuilder::GenerateBoundary() : email_opt->boundary_mixed;
	std::string final_bs = "(" + text_block;

	for (const auto& att : email_opt->attachments)
	{
		final_bs += " (\"application\" \"octet-stream\" (\"name\" \"" + att.file_name + "\") NIL NIL \"base64\" " +
					std::to_string(att.file_size) + ")";
	}

	final_bs += " \"mixed\" (\"boundary\" \"" + mix_bound + "\") NIL NIL)";

	return final_bs;
}

std::string BuildBodystructureFromMimePart(const SmtpClient::MimePart& part)
{
	if (part.IsMultipart())
	{
		std::string result = "(";
		for (size_t i = 0; i < part.children.size(); ++i)
		{
			result += BuildBodystructureFromMimePart(part.children[i]);
		}
		result += " \"" + part.subtype + "\"";
		if (!part.boundary.empty())
		{
			result += " (\"boundary\" \"" + part.boundary + "\")";
		}
		else
		{
			result += " NIL";
		}
		result += " NIL NIL)";
		return result;
	}

	std::string params = "(\"charset\" \"" + part.charset + "\")";
	std::string result = "(\"" + part.type + "\" \"" + part.subtype + "\" " + params + " NIL NIL \"" + part.encoding +
						 "\" " + std::to_string(part.size) + " " + std::to_string(part.lines) + ")";
	return result;
}

std::string GetBodyContent(const Message& msg)
{
	std::ifstream file(msg.raw_file_path);
	if (!file.is_open())
	{
		return "";
	}

	return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

std::string ExtractPartBySection(const std::string& raw_mime, const std::string& boundary, int part_num)
{
	std::string delimiter = "--" + boundary;
	size_t pos = 0;
	int current_part = 0;

	while (current_part < part_num)
	{
		size_t next_delim = raw_mime.find(delimiter, pos);
		if (next_delim == std::string::npos) break;

		size_t part_start = next_delim + delimiter.length();

		if (part_start < raw_mime.length() && raw_mime[part_start] == '\r') part_start++;
		if (part_start < raw_mime.length() && raw_mime[part_start] == '\n') part_start++;

		size_t next_delim2 = raw_mime.find(delimiter, part_start);

		if (current_part == part_num - 1)
		{
			size_t part_end;
			if (next_delim2 != std::string::npos)
			{
				part_end = next_delim2;
			}
			else
			{
				part_end = raw_mime.length();
			}

			size_t result_len = part_end - part_start;
			if (result_len >= 2 && raw_mime[part_end - 2] == '\r' && raw_mime[part_end - 1] == '\n')
			{
				result_len -= 2;
			}
			else if (result_len >= 1 && raw_mime[part_end - 1] == '\n')
			{
				result_len -= 1;
			}

			return raw_mime.substr(part_start, result_len);
		}

		pos = next_delim2;
		current_part++;
	}
	return "";
}

std::vector<std::string> CombineSplitBodySections(const std::vector<std::string>& items)
{
	std::vector<std::string> combined;

	for (size_t i = 0; i < items.size(); ++i)
	{
		std::string item = items[i];

		std::string upper_item = item;
		for (char& c : upper_item)
			c = toupper(c);

		// Check if this is BODY[HEADER.FIELDS (split by spaces)
		if (upper_item.rfind("BODY[HEADER.FIELDS", 0) == 0 || upper_item.rfind("BODY.PEEK[HEADER.FIELDS", 0) == 0)
		{
			bool is_peek = (upper_item.rfind("BODY.PEEK[", 0) == 0);
			size_t prefix_len = is_peek ? 10 : 5; // "BODY.PEEK[" or "BODY["

			std::string section_content = item.substr(prefix_len);

			while (!section_content.empty() && section_content.back() != ']')
			{
				if (i + 1 >= items.size()) break;
				++i;
				section_content += " " + items[i];
			}

			// Ensure closing bracket exists (it was split off by whitespace)
			if (!section_content.empty() && section_content.back() != ']')
			{
				section_content += "]";
			}

			combined.push_back((is_peek ? "BODY.PEEK[" : "BODY[") + section_content);
		}
		else
		{
			combined.push_back(item);
		}
	}

	return combined;
}

std::string GetBodySection(const Message& msg, const std::string& section)
{
	std::string raw_mime = GetBodyContent(msg);
	if (raw_mime.empty()) return "";

	std::string headers, body;
	SmtpClient::MimeParser::SplitHeadersAndBody(raw_mime, headers, body);

	std::string upper_section = section;
	for (char& c : upper_section)
	{
		c = toupper(c);
	}

	if (upper_section == "HEADER")
	{
		return headers + "\r\n\r\n";
	}
	else if (upper_section == "TEXT")
	{
		return body;
	}
	else if (upper_section == "MIME")
	{
		return headers + "\r\n\r\n";
	}
	else if (upper_section.find("HEADER.") == 0)
	{
		return headers + "\r\n\r\n";
	}

	if (isdigit(upper_section[0]))
	{
		std::string top_ct = SmtpClient::MimeParser::GetHeaderValue(headers, "Content-Type:");
		bool is_multipart = (top_ct.find("multipart") != std::string::npos);

		size_t suffix_pos = 0;
		for (size_t i = 0; i < upper_section.length(); ++i)
		{
			if (std::isalpha(upper_section[i]) && i > 0 && upper_section[i - 1] == '.')
			{
				suffix_pos = i - 1;
				break;
			}
		}

		std::string path_str = (suffix_pos > 0) ? upper_section.substr(0, suffix_pos) : upper_section;
		std::string suffix = (suffix_pos > 0) ? upper_section.substr(suffix_pos) : "";

		std::vector<int> path;
		size_t start = 0;
		while (start < path_str.length())
		{
			size_t dot_pos = path_str.find('.', start);
			std::string num_str =
				(dot_pos == std::string::npos) ? path_str.substr(start) : path_str.substr(start, dot_pos - start);
			path.push_back(std::stoi(num_str));
			if (dot_pos == std::string::npos) break;
			start = dot_pos + 1;
		}

		if (!is_multipart)
		{
			if (path.size() > 1 || (!path.empty() && path[0] > 1))
			{
				return "";
			}

			if (path.size() == 1 && path[0] == 1)
			{
				if (suffix == ".HEADER")
				{
					return headers + "\r\n\r\n";
				}
				else if (suffix == ".MIME")
				{
					return headers + "\r\n\r\n";
				}
				return body;
			}

			return "";
		}

		std::string current_body = body;
		std::string current_headers = headers;

		for (size_t i = 0; i < path.size(); ++i)
		{
			int part_num = path[i];

			std::string content_type = SmtpClient::MimeParser::GetHeaderValue(current_headers, "Content-Type:");
			std::string boundary = SmtpClient::MimeParser::ExtractBoundary(content_type);

			if (boundary.empty())
			{
				if (i < path.size() - 1)
				{
					return "";
				}
				break;
			}

			std::string part_content = ExtractPartBySection(current_body, boundary, part_num);
			if (part_content.empty()) return "";

			SmtpClient::MimeParser::SplitHeadersAndBody(part_content, current_headers, current_body);

			// Strip leading CRLF/LF from body after splitting headers
			while (!current_body.empty() && (current_body.front() == '\r' || current_body.front() == '\n'))
			{
				current_body.erase(current_body.begin());
			}
		}

		if (suffix == ".HEADER")
		{
			return current_headers + "\r\n\r\n";
		}
		else if (suffix == ".MIME")
		{
			return current_headers + "\r\n\r\n";
		}
		else if (suffix == ".TEXT")
		{
			return current_body;
		}

		// No suffix - return just body (BODY[1] should return body, not headers)
		return current_body;
	}

	return raw_mime;
}

std::string FormatFlagsResponse(const Message& msg)
{
	std::string f = "(FLAGS (";
	if (msg.is_seen) f += "\\Seen ";
	if (msg.is_deleted) f += "\\Deleted ";
	if (msg.is_draft) f += "\\Draft ";
	if (msg.is_answered) f += "\\Answered ";
	if (msg.is_flagged) f += "\\Flagged ";
	if (msg.is_recent) f += "\\Recent ";
	if (f.back() == ' ') f.pop_back();
	f += "))";

	return f;
}

} // namespace IMAP_UTILS
