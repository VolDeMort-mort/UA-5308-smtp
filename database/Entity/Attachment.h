#pragma once

#include <string>
#include <optional>
#include <cstdint>

struct Attachment
{
    std::optional<int64_t> id;
    int64_t message_id;
    std::string file_name;
    std::optional<std::string> mime_type;
    std::optional<int64_t> file_size;
    std::string storage_path;
    std::string uploaded_at;
};