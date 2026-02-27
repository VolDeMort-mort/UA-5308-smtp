#include "MimeParser.h"
#include "MimeDecoder.h"
#include "../models/Email.h"
#include <sstream>
#include <iostream>

bool MimeParser::parseEmail(const std::string& rawMimeString, Email& outEmailData, ILoggerStrategy& logger) {
    if (rawMimeString.empty()) {
        logger.SpecificLog(PROD, "Parser error: Raw MIME string is empty.");
        return false;
    }

    std::string topHeaders, body;
    splitHeadersAndBody(rawMimeString, topHeaders, body);

    parseMainHeaders(topHeaders, outEmailData);

    std::string contentType = getHeaderValue(topHeaders, "Content-Type:");

    if (contentType.find("multipart") != std::string::npos) {
        std::string boundary = extractBoundary(contentType);

        if (contentType.find("multipart") != std::string::npos) {
            std::string boundary = extractBoundary(contentType);

            std::cout << "DEBUG: Content-Type is: [" << contentType << "]\n";
            std::cout << "DEBUG: Extracted Boundary is: [" << boundary << "]\n";

            if (boundary.empty()) {
                logger.SpecificLog(PROD, "Parser error: Could not extract boundary.");
                return false;
            }

            parseMultipartBody(body, boundary, outEmailData);
        }
        if (boundary.empty()) {
            logger.SpecificLog(PROD, "Parser error: Could not extract boundary.");
            return false;
        }

        parseMultipartBody(body, boundary, outEmailData);
    }
    else {
        outEmailData.plainText = body;
    }

    logger.SpecificLog(DEBUG, "MimeParser: Successfully parsed email from " + outEmailData.sender);
    return true;
}


void MimeParser::splitHeadersAndBody(const std::string& rawBlock, std::string& outHeaders, std::string& outBody) {
    size_t pos = rawBlock.find("\r\n\r\n");
    size_t delimiterLen = 4;

    if (pos == std::string::npos) {
        pos = rawBlock.find("\n\n");
        delimiterLen = 2;
    }

    if (pos != std::string::npos) {
        outHeaders = rawBlock.substr(0, pos);
        outBody = rawBlock.substr(pos + delimiterLen);
    }
    else {
        outHeaders = "";
        outBody = rawBlock;
    }
}

std::string MimeParser::extractBoundary(const std::string& contentTypeHeader) {
    std::string searchKey = "boundary=";
    size_t pos = contentTypeHeader.find(searchKey);

    if (pos == std::string::npos) return ""; 

    pos += searchKey.length();

    std::string boundary = "";

    if (pos < contentTypeHeader.length() && contentTypeHeader[pos] == '"') {
        pos++; 
        size_t endPos = contentTypeHeader.find('"', pos); 
        if (endPos != std::string::npos) {
            boundary = contentTypeHeader.substr(pos, endPos - pos);
        }
        else {
            boundary = contentTypeHeader.substr(pos);
        }
    }
    else {
        size_t endPos = contentTypeHeader.find_first_of(" ;\r\n", pos);
        if (endPos != std::string::npos) {
            boundary = contentTypeHeader.substr(pos, endPos - pos);
        }
        else {
            boundary = contentTypeHeader.substr(pos);
        }
    }

    return boundary;
}




void MimeParser::parseMainHeaders(const std::string& topHeaders, Email& outEmailData) {
    outEmailData.sender = MimeDecoder::decodeEncodedWord(getHeaderValue(topHeaders, "From:"));
    outEmailData.recipient = MimeDecoder::decodeEncodedWord(getHeaderValue(topHeaders, "To:"));
    outEmailData.subject = MimeDecoder::decodeEncodedWord(getHeaderValue(topHeaders, "Subject:"));
}

void MimeParser::parseMultipartBody(const std::string& body, const std::string& boundary, Email& outEmailData) {
    std::string delimiter = "--" + boundary;
    size_t startPos = body.find(delimiter);

    std::cout << "DEBUG: Searching for delimiter: [" << delimiter << "]\n";
    std::cout << "DEBUG: First startPos: " << (startPos == std::string::npos ? -1 : (int)startPos) << "\n";

    while (startPos != std::string::npos) {
        startPos += delimiter.length();

        if (startPos < body.length() && body[startPos] == '\r') startPos++;
        if (startPos < body.length() && body[startPos] == '\n') startPos++;

        if (startPos < body.length() && body.substr(startPos, 2) == "--") {
            std::cout << "DEBUG: Found closing boundary!\n";
            break;
        }

        size_t endPos = body.find(delimiter, startPos);
        std::cout << "DEBUG: Next endPos: " << (endPos == std::string::npos ? -1 : (int)endPos) << "\n";

        if (endPos == std::string::npos) {
            std::cout << "DEBUG: Breaking loop, no end delimiter found.\n";
            break;
        }

        std::string partRaw = body.substr(startPos, endPos - startPos);
        std::cout << "DEBUG: Parsing part of size: " << partRaw.length() << " chars\n";
        processMimePart(partRaw, outEmailData);

        startPos = endPos;
    }
}

void MimeParser::processMimePart(const std::string& partRaw, Email& outEmailData) {
    std::string partHeaders, partBody;
    splitHeadersAndBody(partRaw, partHeaders, partBody);

    std::string partContentType = getHeaderValue(partHeaders, "Content-Type:");

    std::string encoding = getHeaderValue(partHeaders, "Content-Transfer-Encoding:");

    if (encoding.find("quoted-printable") != std::string::npos) {
        partBody = MimeDecoder::decodeQuotedPrintable(partBody);
    }

    if (partContentType.find("text/plain") != std::string::npos) {
        outEmailData.plainText = partBody;
    }

    else if (partContentType.find("text/html") != std::string::npos) {
        outEmailData.htmlText = partBody.substr(0, partBody.find_last_not_of("\r\n") + 1);
    }
    else if (partContentType.find("message/rfc822") != std::string::npos) {
        outEmailData.forwardedEmails.push_back(partBody);
    }
    else if (partContentType.find("application/") != std::string::npos || partHeaders.find("attachment") != std::string::npos) {
        std::string fileName = extractFileName(partContentType);
        outEmailData.addAttachment(fileName, "data/attachments/" + fileName, partBody.length());
    }
}

std::string MimeParser::extractFileName(const std::string& partContentType) {
    std::string fileName = "unknown_file.dat";
    size_t namePos = partContentType.find("name=\"");
    if (namePos != std::string::npos) {
        size_t nameEnd = partContentType.find("\"", namePos + 6);
        if (nameEnd != std::string::npos) {
            fileName = partContentType.substr(namePos + 6, nameEnd - (namePos + 6));
        }
    }
    return fileName;
}

std::string MimeParser::getHeaderValue(const std::string& headers, const std::string& key) {
    size_t pos = 0;

    while (true) {
        pos = headers.find(key, pos);
        if (pos == std::string::npos) return ""; 

        if (pos == 0 || headers[pos - 1] == '\n') {
            break;
        }

        pos += 1;
    }

    pos += key.length();
    std::string val = "";

    while (pos < headers.length()) {
        size_t end = headers.find("\n", pos);
        if (end == std::string::npos) {
            val += headers.substr(pos);
            break;
        }

        size_t actualEnd = (end > pos && headers[end - 1] == '\r') ? end - 1 : end;
        val += headers.substr(pos, actualEnd - pos);

        size_t nextLineStart = end + 1;
        if (nextLineStart < headers.length() &&
            (headers[nextLineStart] == ' ' || headers[nextLineStart] == '\t')) {
            val += " ";
            pos = nextLineStart;
            while (pos < headers.length() && (headers[pos] == ' ' || headers[pos] == '\t')) {
                pos++;
            }
        }
        else {
            break;
        }
    }

    size_t firstChar = val.find_first_not_of(" \t");
    if (firstChar != std::string::npos) {
        val = val.substr(firstChar);
    }
    else {
        val = "";
    }

    size_t lastChar = val.find_last_not_of(" \t");
    if (lastChar != std::string::npos) {
        val = val.substr(0, lastChar + 1);
    }

    return val;
}

