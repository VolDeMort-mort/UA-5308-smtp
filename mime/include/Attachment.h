#pragma once
#include <string>
#include <cstddef>

namespace SmtpClient {

struct Attachment
{
	std::string file_name;
	std::string mime_type;
	std::string base64_data;
	std::size_t file_size;

	Attachment() : file_size(0) {}

	Attachment(std::string name, std::string mime, std::string data)
		: file_name(std::move(name)), mime_type(std::move(mime)), base64_data(std::move(data))
	{
		if (!base64_data.empty())
		{
			std::size_t padding = 0;
			if (base64_data.length() >= 2)
			{
				if (base64_data.back() == '=') padding++;
				if (base64_data[base64_data.length() - 2] == '=') padding++;
			}
			file_size = (base64_data.length() * 3) / 4 - padding;
		}
		else
		{
			file_size = 0;
		}
	}
};

} // namespace SmtpClient
