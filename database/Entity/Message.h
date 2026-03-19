#pragma once

#include <string>
#include <optional>
#include <cstdint>

struct Message
{
    std::optional<int64_t> id;
    int64_t user_id;
    int64_t folder_id;
    int64_t uid;

    std::string raw_file_path;
    int64_t size_bytes = 0;
    std::optional<std::string> mime_structure;

    std::optional<std::string> message_id_header;
    std::optional<std::string> in_reply_to;
    std::optional<std::string> references_header;
    std::string from_address;
    std::optional<std::string> sender_address;
    std::optional<std::string> subject;

    bool is_seen = false;
    bool is_deleted = false;
    bool is_draft = false;
    bool is_answered = false;
    bool is_flagged = false;
    bool is_recent = true;

    std::string internal_date;
    std::optional<std::string> date_header;
};