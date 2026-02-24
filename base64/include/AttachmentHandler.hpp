#pragma once
#include <fstream>
#include <filesystem>
#include <sstream>
#include <regex>
#include "Base64Common.hpp"
#include "Base64StreamEncoder.hpp"
#include "Base64StreamDecoder.hpp"

class AttachmentHandler
{
public:
	static std::string EncodeFile(const std::string& path, const std::string& boundary);
	static bool DecodeFile(const std::string& mime, const std::string& boundary);
	static std::vector<std::string> DecodeAllAttachments(const std::string& mime, const std::string& boundary);

private:
	static std::string get_extension(const std::string& path);
	static std::string get_mimeType(const std::string& path);
};