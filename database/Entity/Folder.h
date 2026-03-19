#pragma once

#include <string>
#include <optional>
#include <cstdint>

struct Folder
{
    std::optional<int64_t> id;
    int64_t user_id;
    std::string name;
    int64_t next_uid = 1;
    bool is_subscribed = false;
};