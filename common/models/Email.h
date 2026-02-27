#pragma once
#include <string>
#include <vector>
#include "Attachment.h"


class Email {
public:
    std::string sender;
    std::string recipient;
    std::string subject;
    std::string date;

    std::string plainText;
    std::string htmlText;

    std::vector<std::string> forwardedEmails;
    std::vector<Attachment> attachments;
    Email() = default;

    void addAttachment(const std::string& name, const std::string& path, size_t size) {
        attachments.push_back(Attachment(name, path, size));
    }

    bool hasAttachments() const {
        return !attachments.empty();
    }
};