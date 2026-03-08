#pragma once

#include "Base64Common.hpp"

#include <vector>
#include <string>
#include <cctype>
#include <iostream>
#include <array>

class Base64Decoder
{
public:
	static std::vector<uint8_t> DecodeBase64(const std::string& encoded_data);
};
