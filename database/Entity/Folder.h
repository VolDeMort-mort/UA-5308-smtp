#pragma once

#include <string>
#include <optional>
#include <cstdint>

struct Folder
{
    //FIX: should be std::optional<int64_t> parent_id;
    std::optional<int64_t> id;
    int64_t user_id;
    std::optional<int64_t> parent_id;
    std::string name;
    int64_t next_uid = 1;
    bool is_subscribed = false;
};