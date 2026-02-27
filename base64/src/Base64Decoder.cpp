#include "Base64Decoder.hpp"

std::vector<uint8_t> Base64Decoder::DecodeBase64(const std::string& encoded_data)
{
	std::string clean_str;
	std::vector<uint8_t> decoded_data;

	for (char c : encoded_data)
	{
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=')
		{
			clean_str += c;
		}
		else
		{
			throw std::runtime_error(std::string("Invalid Base64 symbol: ") + c);
		}
	}

	if (!(clean_str.length() % 4 == 0))
	{
		throw std::runtime_error("Str is not in Base64 format");
	}

	for (std::size_t i = 0; i < clean_str.length(); i += 4)
	{
		char c0 = clean_str[i];
		char c1 = clean_str[i + 1];
		char c2 = clean_str[i + 2];
		char c3 = clean_str[i + 3];

		const auto& decode_table = Base64::get_decode_table();

		int value0 = decode_table[(unsigned char) c0];
		int value1 = decode_table[(unsigned char) c1];
		int value2 = (c2 == '=') ? 0 : decode_table[(unsigned char) c2];
		int value3 = (c3 == '=') ? 0 : decode_table[(unsigned char) c3];

		if (value0 < 0 || value1 < 0 || (c2 != '=' && value2 < 0) || (c3 != '=' && value3 < 0))
		{
			throw std::runtime_error("Invalid Base64 character");
		}

		uint32_t block = (value0 << 18) | (value1 << 12) | (value2 << 6) | value3;

		decoded_data.push_back((block >> 16) & 0xFF);

		if (c2 != '=')
		{
			decoded_data.push_back((block >> 8) & 0xFF);
		}
			
		if (c3 != '=')
		{
			decoded_data.push_back(block & 0xFF);
		}
	}
	return decoded_data;
}