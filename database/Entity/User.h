#pragma once

#include <string>
#include <optional>
#include <cstdint>

struct User
{
    std::optional<int64_t> id;
    std::string username;
    std::string password_hash;
    std::optional<std::string> first_name;
    std::optional<std::string> last_name;
    std::optional<std::string> birthdate;
    std::optional<std::string> avatar_b64;
};