#include "MimeDecoder.h"
#include <regex>

char MimeDecoder::hexToChar(const std::string& hexStr) {
    return static_cast<char>(std::stoi(hexStr, nullptr, 16));
}

std::string MimeDecoder::decodeEncodedWord(const std::string& input) {
    std::string result = input;

    size_t startPos = result.find("=?");

    while (startPos != std::string::npos) {
        size_t endPos = result.find("?=", startPos);
        if (endPos == std::string::npos) break; 

        std::string block = result.substr(startPos, endPos - startPos + 2);

        size_t q1 = block.find('?', 2);
        size_t q2 = (q1 != std::string::npos) ? block.find('?', q1 + 1) : std::string::npos;

        if (q1 != std::string::npos && q2 != std::string::npos) {
            std::string encoding = block.substr(q1 + 1, q2 - q1 - 1);
            std::string encodedText = block.substr(q2 + 1, block.length() - q2 - 3); 

            std::string decodedText = "";

            if (encoding == "Q" || encoding == "q") {
                for (size_t i = 0; i < encodedText.length(); ++i) {
                    if (encodedText[i] == '_') {
                        decodedText += ' ';
                    }
                    else if (encodedText[i] == '=' && i + 2 < encodedText.length()) {
                        std::string hex = encodedText.substr(i + 1, 2);
                        decodedText += hexToChar(hex);
                        i += 2;
                    }
                    else {
                        decodedText += encodedText[i]; 
                    }
                }
            }
            else if (encoding == "B" || encoding == "b") {
                decodedText = "[Base64_Decoded]";
            }

            result.replace(startPos, block.length(), decodedText);

            startPos = result.find("=?", startPos + decodedText.length());
        }
        else {
            startPos = result.find("=?", startPos + 2);
        }
    }

    return result;
}

std::string MimeDecoder::decodeQuotedPrintable(const std::string& input) {
    std::string decoded = "";
    size_t i = 0;
    while (i < input.length()) {
        if (input[i] == '=') {
            if (i + 1 < input.length() && (input[i + 1] == '\r' || input[i + 1] == '\n')) {
                i++;
                if (i < input.length() && input[i] == '\r') i++;
                if (i < input.length() && input[i] == '\n') i++;
            }
            else if (i + 2 < input.length()) {
                std::string hex = input.substr(i + 1, 2);
                try {
                    decoded += hexToChar(hex);
                    i += 3; 
                }
                catch (...) {
                    decoded += '='; 
                    i++;
                }
            }
            else {
                decoded += '=';
                i++;
            }
        }
        else {
            decoded += input[i];
            i++;
        }
    }
    return decoded;
}