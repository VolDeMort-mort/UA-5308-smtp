#pragma once
#include <vector>
#include <string>
#include <stdexcept>
#include "Base64Common.hpp"
#include "AttachmentConfig.hpp"

class Base64Encoder
{
public:
	static std::string EncodeBase64(const std::vector< uint8_t>& data);

private:
	static void ChunkBytes(const std::vector<uint8_t>& data, std::string& output);
};
