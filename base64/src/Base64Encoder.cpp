#include "Base64Encoder.hpp"

void Base64Encoder::ChunkBytes(const std::vector<uint8_t>& data, std::string& output)
{
	std::size_t data_size = data.size();
	std::size_t line_length = 0;

	for (std::size_t i = 0; i < data_size; i += 3)
	{
		uint8_t byte0 = data[i];
		uint8_t byte1 = (i + 1 < data_size) ? data[i + 1] : 0;
		uint8_t byte2 = (i + 2 < data_size) ? data[i + 2] : 0;

		uint32_t block = (byte0 << 16) | (byte1 << 8) | byte2;

		char char0 = Base64::alphabet[(block >> 18) & 0x3F];
		char char1 = Base64::alphabet[(block >> 12) & 0x3F];
		char char2 = (i + 1 < data_size) ? Base64::alphabet[(block >> 6) & 0x3F] : '=';
		char char3 = (i + 2 < data_size) ? Base64::alphabet[block & 0x3F] : '=';

		output += char0;
		output += char1;
		output += char2;
		output += char3;

		line_length += 4;
		bool more_data = (i + 3 < data_size);

		if (line_length >= 76 && more_data)
		{
			output += "\r\n";
			line_length = 0;
		}

	}
}

std::string Base64Encoder:: EncodeBase64(const std::vector< uint8_t>& data)
{
	std::string encoded_output;
	uint32_t maxfileSize = 15 * 1024 * 1024;

	if (data.size() > maxfileSize)
	{
		throw std::runtime_error("File too large");
	}

	ChunkBytes(data, encoded_output);
	return encoded_output;

}