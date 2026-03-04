#pragma once

#include <string>
#include <optional>
#include <cstdint>

struct User
{
    std::optional<int64_t> id;
    std::string username;
    std::string password_hash;
};