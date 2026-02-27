#include <string>
#include <vector>

#include "ImapCommand.hpp"

namespace IMAP_UTILS
{

std::string ToUpper(std::string str);

std::vector<std::string> SplitArgs(const std::string& str);

std::string JoinArgs(const std::vector<std::string>& args);

ImapCommandType StringToCommandType(const std::string& cmd);

std::string CommandTypeToString(const ImapCommandType& type);

} // namespace IMAP_UTILS