#include <cstdint>
#include <string>
#include <vector>

#include "Entity/Message.h"
#include "ImapCommand.hpp"

namespace IMAP_UTILS
{

std::string ToUpper(std::string str);

std::vector<std::string> SplitArgs(const std::string& str);

std::string JoinArgs(const std::vector<std::string>& args);

ImapCommandType StringToCommandType(const std::string& cmd);

std::string CommandTypeToString(const ImapCommandType& type);

std::string TrimParentheses(const std::string& str);

std::vector<int64_t> ParseSequenceSet(const std::string& sequenceSet, int64_t maxValue = INT64_MAX);

// should be moved to repository layer
void SortMessagesByTimeDescending(std::vector<Message>& messages);

} // namespace IMAP_UTILS
