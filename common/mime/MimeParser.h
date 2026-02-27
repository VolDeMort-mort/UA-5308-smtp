#pragma once
#include <string>
#include "../models/Email.h"
#include "ILoggerStrategy.h"


class MimeParser {
public:
    static bool parseEmail(const std::string& rawMimeString, Email& outEmailData, ILoggerStrategy& logger);

private:
    static void splitHeadersAndBody(const std::string& rawBlock, std::string& outHeaders, std::string& outBody);

    static std::string extractBoundary(const std::string& contentTypeHeader);

    static std::string getHeaderValue(const std::string& headers, const std::string& key);

    static void parseMainHeaders(const std::string& topHeaders, Email& outEmailData);

    static void parseMultipartBody(const std::string& body, const std::string& boundary, Email& outEmailData);

    static void processMimePart(const std::string& partRaw, Email& outEmailData);

    static std::string extractFileName(const std::string& partContentType);
};
