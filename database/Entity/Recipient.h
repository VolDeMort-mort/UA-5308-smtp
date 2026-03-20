#pragma once

#include <string>
#include <optional>
#include <cstdint>

enum class RecipientType
{
    To,
    Cc,
    Bcc,
    ReplyTo
};

struct Recipient
{
    std::optional<int64_t> id;
    int64_t message_id;
    std::string address;
    RecipientType type = RecipientType::To;

    static std::string typeToString(RecipientType type);
    static RecipientType typeFromString(const std::string& str);
};