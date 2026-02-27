#include "MimeBuilder.h"
#include <random>
#include <chrono>

std::string MimeBuilder::generateBoundary() {
    std::string boundary = "----=_Part_";

    auto now = std::chrono::system_clock::now().time_since_epoch(); 
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    boundary += std::to_string(millis) + "_";

    const std::string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    std::random_device rd;  
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, charset.size() - 1);

    for (int i = 0; i < 16; ++i) {
        boundary += charset[dist(gen)];
    }

    return boundary;
}

bool MimeBuilder::buildEmail(const Email& emailData, std::string& outMimeString, ILoggerStrategy& logger) {
    if (!validateEmail(emailData, logger)) {
        return false;
    }

    std::string boundary = generateBoundary();

    appendMainHeaders(emailData, boundary, outMimeString);
    appendTextPart(emailData.plainText, boundary, outMimeString);
    appendAttachments(emailData.attachments, boundary, outMimeString);

    outMimeString += "--" + boundary + "--\r\n";
    
    logger.SpecificLog(DEBUG, "Email successfully built for " + emailData.recipient);

    return true;
}


bool MimeBuilder::validateEmail(const Email& emailData, ILoggerStrategy& logger) {
    if (emailData.sender.empty()) {
        logger.SpecificLog(PROD, "Failed to build email: Sender address is empty.");
        return false;
    }
    if (emailData.recipient.empty()) {
        logger.SpecificLog(PROD, "Failed to build email: Recipient address is empty.");
        return false;
    }
    return true;
}

void MimeBuilder::appendMainHeaders(const Email& emailData, const std::string& boundary, std::string& out) {
    out += "From: " + emailData.sender + "\r\n";
    out += "To: " + emailData.recipient + "\r\n";

    if (!emailData.subject.empty()) {
        out += "Subject: " + emailData.subject + "\r\n";
    }

    out += "MIME-Version: 1.0\r\n";
    out += "Content-Type: multipart/mixed; boundary=\"" + boundary + "\"\r\n";
    out += "\r\n";
}

void MimeBuilder::appendTextPart(const std::string& plainText, const std::string& boundary, std::string& out) {
    if (plainText.empty()) return;

    out += "--" + boundary + "\r\n";
    out += "Content-Type: text/plain; charset=\"utf-8\"\r\n";
    out += "Content-Transfer-Encoding: 8bit\r\n";
    out += "\r\n";
    out += plainText + "\r\n";
}

void MimeBuilder::appendAttachments(const std::vector<Attachment>& attachments, const std::string& boundary, std::string& out) {
    for (const auto& attachment : attachments) {
        out += "--" + boundary + "\r\n";
        out += "Content-Type: application/octet-stream; name=\"" + attachment.fileName + "\"\r\n";
        out += "Content-Disposition: attachment; filename=\"" + attachment.fileName + "\"\r\n";
        out += "Content-Transfer-Encoding: base64\r\n";
        out += "\r\n";

        //TODO: attach encoded file
        out += "VGhpcyBpcyBhIGZha2UgYmFzZTY0IGZpbGU... (BASE64 file)\r\n";
    }
}

