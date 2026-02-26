#pragma once

#include <string>
#include <optional>
#include <cstdint>

struct Recipient 
{
    std::optional<int64_t> id;
    int64_t message_id;
    std::string address;
};