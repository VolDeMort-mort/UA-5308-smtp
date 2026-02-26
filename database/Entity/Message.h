#pragma once

#include <string>
#include <optional>
#include <cstdint>

enum class MessageStatus
{
    Draft,
    Sent,
    Received,
    Deleted,
    Failed
};

struct Message
{
    std::optional<int64_t> id;
    int64_t user_id;
    std::optional<int64_t> folder_id;

    std::string subject;
    std::string body;
    std::string receiver;

    MessageStatus status = MessageStatus::Draft;
    bool is_seen = false;
    bool is_starred = false;
    bool is_important = false;

    std::string created_at;

    static std::string statusToString(MessageStatus status);
    static MessageStatus statusFromString(const std::string& str);
};