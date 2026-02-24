#include "AttachmentHandler.hpp"

std::string AttachmentHandler::get_extension(const std::string& path)
{
	std::size_t position = path.find_last_of('.');

	if (position == std::string::npos)
		return "";

	return path.substr(position);
}

std::string AttachmentHandler::get_mimeType(const std::string& path)
{
	std::string extension = get_extension(path);

	auto iterator = Base64::mime_map.find(extension);
	if (iterator != Base64::mime_map.end())
	{
		return iterator->second;
	}	
	return "application/octet-stream";
}


std::string AttachmentHandler::EncodeFile(const std::string& path, const std::string& boundary)
{
	std::string mime;
	std::string content_type = get_mimeType(path);
	std::string file_name = std::filesystem::path(path).filename().string();

	mime += "--" + boundary + "\r\n";
	mime += "Content-Type: " + content_type + "; name=\"" + file_name + "\"\r\n";
	mime += "Content-Transfer-Encoding: base64\r\n";
	mime += "Content-Disposition: attachment; filename=\"" + file_name + "\"\r\n\r\n";

	std::ifstream file(path, std::ios::binary);
	if (!file)
		throw std::runtime_error("Cannot open file");

	Base64StreamEncoder encoder;
	std::ostringstream encoded_stream;
	char buffer[3 * 1024];
	while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0)
	{
		encoder.EncodeStream(reinterpret_cast<const uint8_t*>(buffer), file.gcount(), encoded_stream);
	}
	encoder.Finalize(encoded_stream);

	mime += encoded_stream.str();

	return mime;

}

bool AttachmentHandler::DecodeFile(const std::string& mime, const std::string& boundary)
{
	if (mime.empty())
		return false;

	std::istringstream stream(mime);
	std::string line;

	Base64StreamDecoder decoder;
	std::ofstream out_file;

	bool in_headers = true;
	bool in_body = false;
	bool is_base64 = false;
	std::string filename;

	std::string boundary_line = "--" + boundary;
	std::string end_boundary = boundary_line + "--";

	while (std::getline(stream, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();

		if (line == boundary_line || line == end_boundary)
		{
			if (in_body && is_base64 && out_file.is_open())
			{
				decoder.Finalize(out_file);
				out_file.close();
			}

			if (line == end_boundary)
				break;

			in_headers = true;
			in_body = false;
			is_base64 = false;
			filename.clear();
			continue;
		}

		if (in_headers)
		{
			if (line.empty())
			{
				in_headers = false;
				in_body = is_base64;

				if (in_body && !filename.empty())
				{
					out_file.open(filename, std::ios::binary);
					if (!out_file)
						throw std::runtime_error("Cannot create output file: " + filename);
				}
				continue;
			}

			if (line.find("Content-Transfer-Encoding:") != std::string::npos)
			{
				if (line.find("base64") != std::string::npos)
					is_base64 = true;
			}

			if (line.find("Content-Disposition:") != std::string::npos)
			{
				std::smatch match;
				std::regex rx("filename=\"([^\"]+)\"");
				if (std::regex_search(line, match, rx))
				{
					filename = match[1];
				}
			}
			continue;
		}
		if (in_body && is_base64 && out_file.is_open())
		{
			decoder.DecodeStream(line.c_str(), line.size(), out_file);
		}
	}

	return true;
}

std::vector<std::string> AttachmentHandler::DecodeAllAttachments(const std::string& mime, const std::string& boundary)
{
	std::istringstream stream(mime);
	std::string line;
	std::vector<std::string> created_files;

	bool in_headers = true;
	bool in_body = false;
	bool is_base64 = false;
	std::string filename;

	Base64StreamDecoder decoder;
	std::ofstream out_file;

	std::string boundary_line = "--" + boundary;
	std::string end_boundary = boundary_line + "--";

	while (std::getline(stream, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();

		if (line == boundary_line || line == end_boundary)
		{
			if (in_body && is_base64)
			{
				decoder.Finalize(out_file);
				out_file.close();
			}

			if (line == end_boundary)
				break;

			in_headers = true;
			in_body = false;
			is_base64 = false;
			filename.clear();
			continue;
		}

		if (in_headers)
		{
			if (line.empty())
			{
				in_headers = false;
				in_body = is_base64;
				if (in_body && !filename.empty())
				{
					out_file.open(filename, std::ios::binary);
					created_files.push_back(filename);
				}
				continue;
			}

			if (line.find("Content-Transfer-Encoding:") != std::string::npos)
			{
				if (line.find("base64") != std::string::npos)
				{
					is_base64 = true;
				}
			}

			if (line.find("Content-Disposition:") != std::string::npos)
			{
				std::smatch match;
				std::regex rx("filename=\"([^\"]+)\"");
				if (std::regex_search(line, match, rx))
				{
					filename = match[1];
				}
			}

			continue;
		}

		if (in_body && is_base64)
		{
			decoder.DecodeStream(line.c_str(), line.size(), out_file);
		}
	}

	return created_files;
}
