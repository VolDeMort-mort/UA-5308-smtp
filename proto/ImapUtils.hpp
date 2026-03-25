#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "Entity/Message.h"
#include "ImapCommand.hpp"
#include "MimeParser.h"
#include "MimePart.h"

class MessageRepository;

namespace IMAP_UTILS
{

std::string ToUpper(std::string str);

std::vector<std::string> SplitArgs(const std::string& str);

std::string JoinArgs(const std::vector<std::string>& args);

ImapCommandType StringToCommandType(const std::string& cmd);

std::string CommandTypeToString(const ImapCommandType& type);

std::string TrimParentheses(const std::string& str);

std::vector<int64_t> ParseSequenceSet(const std::string& sequenceSet, int64_t maxValue = INT64_MAX);

// example: "2025-01-15 14:30:45" -> "Wed, 15 Jan 2025 14:30:45 +0000")
std::string DateToEmlDate(const std::string& str);

// example: "2025-01-15 14:30:45" -> "15-Jan-2025 14:30:45 +0000"
std::string DateToIMAPInternal(const std::string& str);

// should be moved to repository layer
void SortMessagesByTimeDescending(std::vector<Message>& messages);

std::optional<SmtpClient::Email> GetParsedEmail(const Message& msg, ILogger& logger);

std::string BuildEnvelope(const Message& msg, const std::optional<SmtpClient::Email>& email_opt,
						  MessageRepository& messRepo);

std::string BuildBodystructure(const Message& msg, const std::optional<SmtpClient::Email>& email_opt,
							   const std::optional<SmtpClient::MimePart>& mime_part = std::nullopt);

std::optional<SmtpClient::MimePart> GetParsedMimePart(const Message& msg, ILogger& logger);

std::string BuildBodystructureFromMimePart(const SmtpClient::MimePart& part);

std::string GetBodyContent(const Message& msg);

std::string GetBodySection(const Message& msg, const std::string& section);

std::vector<std::string> CombineSplitBodySections(const std::vector<std::string>& items);

} // namespace IMAP_UTILS
