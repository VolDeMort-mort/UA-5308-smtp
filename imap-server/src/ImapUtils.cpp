#include "ImapUtils.hpp"

#include <algorithm>
#include <fstream>
#include <numeric>
#include <sstream>
#include <unordered_map>

#include "Entity/Recipient.h"
#include "MimeBuilder.h"
#include "Repository/MessageRepository.h"

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
		{"IDLE", ImapCommandType::Idle},
		{"COPY", ImapCommandType::Copy},
		{"MOVE", ImapCommandType::Move},
		{"EXPUNGE", ImapCommandType::Expunge},
		{"UID", ImapCommandType::UidFetch},
		{"SUBSCRIBE", ImapCommandType::Subscribe},
		{"UNSUBSCRIBE", ImapCommandType::Unsubscribe},
		{"CLOSE", ImapCommandType::Close},
		{"CHECK", ImapCommandType::Check},
		{"STARTTLS", ImapCommandType::StartTLS}};

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
		{ImapCommandType::Idle, "IDLE"},
		{ImapCommandType::Copy, "COPY"},
		{ImapCommandType::Move, "MOVE"},
		{ImapCommandType::Expunge, "EXPUNGE"},
		{ImapCommandType::UidFetch, "UID FETCH"},
		{ImapCommandType::UidStore, "UID STORE"},
		{ImapCommandType::UidCopy, "UID COPY"},
		{ImapCommandType::Subscribe, "SUBSCRIBE"},
		{ImapCommandType::Unsubscribe, "UNSUBSCRIBE"},
		{ImapCommandType::Close, "CLOSE"},
		{ImapCommandType::Check, "CHECK"},
		{ImapCommandType::StartTLS, "STARTTLS"}};

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
	auto rec_to = std::find_if(all_receipients.begin(), all_receipients.end(),
							   [](Recipient rec) { return rec.type == RecipientType::To; });
	auto rec_cc = std::find_if(all_receipients.begin(), all_receipients.end(),
							   [](Recipient rec) { return rec.type == RecipientType::Cc; });
	auto rec_bcc = std::find_if(all_receipients.begin(), all_receipients.end(),
								[](Recipient rec) { return rec.type == RecipientType::Bcc; });

	std::string env = "(";
	env += "\"" + (email_opt ? date : DateToEmlDate(date)) + "\" ";
	env += "\"" + subject + "\" ";
	env += FormatAddress(sender_email) + " ";
	env += FormatAddress(sender_email) + " ";
	env += FormatAddress(sender_email) + " ";
	env += (rec_to != all_receipients.end()) ? (FormatAddress(rec_to->address) + " ") : "NIL ";
	env += (rec_cc != all_receipients.end()) ? (FormatAddress(rec_cc->address) + " ") : "NIL ";
	env += (rec_bcc != all_receipients.end()) ? (FormatAddress(rec_bcc->address) + " ") : "NIL ";
	env += (!in_reply_to.empty() && in_reply_to != "NIL") ? ("<" + in_reply_to + "> ") : "NIL ";
	env += "\"" + message_id + "\"";
	env += ")";
	return env;
}

std::string BuildBodystructure(const Message& msg, const std::optional<SmtpClient::Email>& email_opt)
{
	if (email_opt && email_opt->HasAttachments())
	{
		std::string boundary = SmtpClient::MimeBuilder::GenerateBoundary();
		std::string bs = "((\"text\" \"plain\" (\"charset\" \"utf-8\") NIL NIL \"7bit\" ";
		bs += std::to_string(msg.size_bytes) + " 0) ";

		for (const auto& att : email_opt->attachments)
		{
			bs += "(\"application\" \"octet-stream\" (\"name\" \"" + att.file_name + "\") NIL NIL \"base64\" ";
			bs += std::to_string(att.file_size) + " 0) ";
		}

		bs += "\"multipart/mixed\" (\"boundary\" \"" + boundary + "\") NIL NIL)";
		return bs;
	}
	else if (email_opt && email_opt->HasHtml())
	{
		// can`t extract boundary from SmtpClient::Email class
		// also information about original encoding isn`t being saved
		std::string boundary = SmtpClient::MimeBuilder::GenerateBoundary();
		std::string bs = "((\"text\" \"plain\" (\"charset\" \"utf-8\") NIL NIL \"7bit\" ";
		bs += std::to_string(email_opt->plain_text.size()) + " 0) ";
		bs += "(\"text\" \"html\" (\"charset\" \"utf-8\") NIL NIL \"7bit\" ";
		bs += std::to_string(email_opt->html_text.size()) + " 0) ";
		bs += "\"multipart/alternative\" (\"boundary\" \"" + boundary + "\") NIL NIL)";
		return bs;
	}
	else
	{
		size_t body_size = email_opt ? email_opt->plain_text.size() : msg.size_bytes;
		return "(\"text\" \"plain\" (\"charset\" \"utf-8\") NIL NIL \"7bit\" " + std::to_string(body_size) + " 0)";
	}
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

} // namespace IMAP_UTILS
