#pragma once

#include <string>
#include <optional>

struct Folder 
{
    std::optional<int64_t> id;
    int64_t user_id;
    std::string name;
};