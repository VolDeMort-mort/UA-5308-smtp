#pragma once

#include "Base64Common.hpp"
#include "Base64StreamEncoder.hpp"
#include "Base64StreamDecoder.hpp"

#include <fstream>
#include <filesystem>
#include <sstream>
#include <regex>

class AttachmentHandler
{
public:
	static std::string EncodeFile(const std::string& path);
	static bool DecodeFile(const std::string& mime, const std::string& boundary);
	static std::vector<std::string> DecodeAllAttachments(const std::string& mime, const std::string& boundary);

private:
	struct MIMEPart
	{
		std::string filename;
		bool is_base64 = false;
		std::vector<std::string> body_lines;
	};

	static std::vector<MIMEPart> ParseMIME(const std::string& mime, const std::string& boundary);
	static void WriteToFile(const MIMEPart& part);
};
