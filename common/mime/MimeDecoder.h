#pragma once
#include <string>

class MimeDecoder {
public:
    static std::string decodeEncodedWord(const std::string& input);
    static std::string decodeQuotedPrintable(const std::string& input);
private:
    static char hexToChar(const std::string& hexStr);
};