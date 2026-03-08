#include "AttachmentHandler.hpp"

std::string AttachmentHandler::EncodeFile(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file)
	{
		throw std::runtime_error("Cannot open file");
	}

	Base64StreamEncoder encoder;
	std::ostringstream encoded_stream;

	char buffer[AConfig::BASE64_CHUNK_SIZE];
	while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0)
	{
		encoder.EncodeStream(reinterpret_cast<const uint8_t*>(buffer), file.gcount(), encoded_stream);
	}

	encoder.Finalize(encoded_stream);

	return encoded_stream.str();
}

std::vector<AttachmentHandler::MIMEPart> AttachmentHandler::ParseMIME(const std::string& mime, const std::string& boundary)
{
	std::istringstream stream(mime);
	std::string line;
	std::vector<MIMEPart> parts;

	std::string boundary_line = "--" + boundary;
	std::string end_boundary = boundary_line + "--";

	bool in_headers = true;
	MIMEPart current;

	while (std::getline(stream, line))
	{
		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}

		if (line == boundary_line || line == end_boundary)
		{
			if (!current.filename.empty() || current.is_base64)
			{
				parts.push_back(current);
			}
			current = {};
			in_headers = true;

			if (line == end_boundary)
			{
				break;
			}
			continue;
		}

		if (in_headers)
		{
			if (line.empty())
			{
				in_headers = false;
				continue;
			}

			if (line.find("Content-Transfer-Encoding:") != std::string::npos)
			{
				if (line.find("base64") != std::string::npos)
				{
					current.is_base64 = true;
				}
			}

			if (line.find("Content-Disposition:") != std::string::npos)
			{
				std::smatch match;
				std::regex rx("filename=\"([^\"]+)\"");
				if (std::regex_search(line, match, rx))
				{
					current.filename = match[1];
				}
			}
			continue;
		}

		if (current.is_base64)
		{
			current.body_lines.push_back(line);
		}
	}

	return parts;
}

void AttachmentHandler::WriteToFile(const AttachmentHandler::MIMEPart& part)
{
	std::ofstream out_file(part.filename, std::ios::binary);
	if (!out_file)
	{
		throw std::runtime_error("Cannot create output file: " + part.filename);
	}

	Base64StreamDecoder decoder;

	for (const auto& line : part.body_lines)
	{
		decoder.DecodeStream(line.c_str(), line.size(), out_file);
	}

	decoder.Finalize(out_file);
}

bool AttachmentHandler::DecodeFile(const std::string& mime, const std::string& boundary)
{
	if (mime.empty())
	{
		return false;
	}

	for (const auto& part : ParseMIME(mime, boundary))
	{
		if (!part.is_base64 || part.filename.empty())
		{
			continue;
		}

		WriteToFile(part);
	}
	return true;
}

std::vector<std::string> AttachmentHandler::DecodeAllAttachments(const std::string& mime, const std::string& boundary)
{
	std::vector<std::string> created_files;

	for (const auto& part : ParseMIME(mime, boundary))
	{
		if (!part.is_base64 || part.filename.empty())
		{
			continue;
		}

		WriteToFile(part);
		created_files.push_back(part.filename);
	}
	return created_files;
}
