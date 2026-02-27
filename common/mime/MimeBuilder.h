#pragma once
#include <string>
#include "../models/Email.h"
#include "../logger/ILoggerStrategy.h"

class MimeBuilder {
public:
//  requirements from https://www.rfc-editor.org/rfc/rfc2046#section-5.1.1
//  length 1-70 
//  allowed to use numbers, letters, space, and symbols: ' ( ) + _ , - . / : = ? 
//  Boundary cant end on space
//  Boundary  must not appear in data
//  "...result of an algorithm designed to produce boundary
//  delimiters with a very low probability of already existing in the data..."
//  "...without having to prescan the data."
    static std::string generateBoundary();

//  builds a full MIME multipart/mixed email string ready for SMTP transport.
//  true if the email was successfully built.
    static bool buildEmail(const Email& emailData, std::string& outMimeString, ILoggerStrategy& logger);
private:
    static bool validateEmail(const Email& emailData, ILoggerStrategy& logger);
    static void appendMainHeaders(const Email& emailData, const std::string& boundary, std::string& out);
    static void appendTextPart(const std::string& plainText, const std::string& boundary, std::string& out);
    static void appendAttachments(const std::vector<Attachment>& attachments, const std::string& boundary, std::string& out);
};