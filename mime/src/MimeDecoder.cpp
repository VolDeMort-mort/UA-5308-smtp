#include "../include/MimeDecoder.h"

#include <string>
#include <vector>

#include "../../base64/include/Base64Decoder.hpp"

namespace SmtpClient {

namespace {

bool IsHex(unsigned char c)
{
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

char InternalHexToChar(const std::string& hex_str)
{
	if (hex_str.length() < 2) return '?';

	char c = 0;
	for (int i = 0; i < 2; ++i)
	{
		char h = hex_str[i];
		c <<= 4;
		if (h >= '0' && h <= '9')      c |= (h - '0');
		else if (h >= 'A' && h <= 'F') c |= (h - 'A' + 10);
		else if (h >= 'a' && h <= 'f') c |= (h - 'a' + 10);
		else return '?';
	}
	return c;
}

bool HasQpArtifacts(const std::string& text)
{
	for (size_t i = 0; i + 2 < text.length(); ++i)
	{
		if (text[i] == '=' && IsHex(static_cast<unsigned char>(text[i + 1]))
							&& IsHex(static_cast<unsigned char>(text[i + 2])))
		{
			return true;
		}
	}
	return false;
}

std::string DecodeQEncoding(const std::string& text)
{
	std::string decoded;
	// decoded text whould not be longer that encoded
	// so i don't do not necessary reallocations after
	decoded.reserve(text.length());

	for (size_t i = 0; i < text.length(); ++i)
	{
		if (text[i] == '_')
		{
			decoded += ' ';
		}
		else if (text[i] == '=')
		{
			if (i + 2 < text.length()
				&& IsHex(static_cast<unsigned char>(text[i + 1]))
				&& IsHex(static_cast<unsigned char>(text[i + 2])))
			{
				decoded += InternalHexToChar(text.substr(i + 1, 2));
				i += 2;
			}
			else
			{
				decoded += '=';
			}
		}
		else
		{
			decoded += text[i];
		}
	}
	return decoded;
}

} // anonymous namespace

std::string MimeDecoder::DecodeEncodedWord(const std::string& input)
{
	std::string result;
	size_t pos = 0;
	bool found_any_encoded = false;

	// looking for pattern =?charset?encoding?data?= 

	while (pos < input.length())
	{
		size_t start = input.find("=?", pos);

		if (start == std::string::npos)
		{
			std::string tail = input.substr(pos);
			if (!found_any_encoded || tail.find_first_not_of(" \t\r\n") != std::string::npos)
				result += tail;
			break;
		}

		std::string gap = input.substr(pos, start - pos);
		if (!found_any_encoded || gap.find_first_not_of(" \t\r\n") != std::string::npos)
			result += gap;

		size_t q1 = input.find('?', start + 2);
		if (q1 == std::string::npos) { result += input.substr(start); break; }

		size_t q2 = input.find('?', q1 + 1);
		if (q2 == std::string::npos) { result += input.substr(start); break; }

		size_t end = input.find("?=", q2 + 1);
		if (end == std::string::npos) { result += input.substr(start); break; }

		std::string encoding = input.substr(q1 + 1, q2 - q1 - 1);
		std::string text     = input.substr(q2 + 1, end - q2 - 1);

		std::string decoded_part;

		if (encoding == "Q" || encoding == "q")
		{
			decoded_part = DecodeQEncoding(text);
		}
		else if (encoding == "B" || encoding == "b")
		{
			try
			{
				std::vector<uint8_t> bytes = Base64Decoder::DecodeBase64(text);
				decoded_part = std::string(bytes.begin(), bytes.end());
			}
			catch (...)
			{
				decoded_part = text;
			}
		}
		else
		{
			decoded_part = text;
		}

		result += decoded_part;
		found_any_encoded = true;
		pos = end + 2;
	}

	if (!found_any_encoded && HasQpArtifacts(result))
		return DecodeQEncoding(result);

	return result;
}

std::string MimeDecoder::DecodeQuotedPrintable(const std::string& input)
{
	std::string decoded;
	decoded.reserve(input.length());

	size_t i = 0;
	while (i < input.length())
	{
		if (input[i] == '=')
		{

			if (i + 1 < input.length() && (input[i + 1] == '\r' || input[i + 1] == '\n'))
			{
				i++;
				if (i < input.length() && input[i] == '\r') i++;
				if (i < input.length() && input[i] == '\n') i++;
			}
			else if (i + 2 < input.length()
					 && IsHex(static_cast<unsigned char>(input[i + 1]))
					 && IsHex(static_cast<unsigned char>(input[i + 2])))
			{
				decoded += InternalHexToChar(input.substr(i + 1, 2));
				i += 3;
			}
			else
			{
				decoded += '=';
				i++;
			}
		}
		else
		{
			decoded += input[i++];
		}
	}
	return decoded;
}

} // namespace SmtpClient
