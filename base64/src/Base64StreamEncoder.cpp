#include "Base64StreamEncoder.hpp"

void Base64StreamEncoder::EncodePart(const uint8_t* data, std::size_t size, std::ostream& out)
{
	uint8_t byte0 = data[0];
	uint8_t byte1 = size  > 1 ? data[1] : 0;
	uint8_t byte2 = size > 2 ? data[2] : 0;

	uint32_t block = (byte0 << 16) | (byte1 << 8) | byte2;

	char chars[4] =
	{
		Base64::alphabet[(block >> 18) & 0x3F],
		Base64::alphabet[(block >> 12) & 0x3F],
		size > 1 ? Base64::alphabet[(block >> 6) & 0x3F] : '=',
		size > 2 ? Base64::alphabet[block & 0x3F] : '='
	};

	for (char c : chars)
	{
		out << c;
		if (++m_line_length == 76)
		{
			out << "\r\n";
			m_line_length = 0;
		}

	}
}

void Base64StreamEncoder::EncodeStream(const uint8_t* data, std::size_t size, std::ostream& out)
{
	if (m_total_encoded + size > AConfig::MAX_FILE_SIZE)
	{
		throw std::runtime_error("File too large for Base64 encoding");
	}

	m_total_encoded += size;
	std::size_t i = 0;

	if (m_left_size > 0)
	{
		while (m_left_size < 3 && i < size)
		{
			if (m_left_size >= 2)
			{
				break;
			}
			m_left[m_left_size++] = data[i++];
		}

		if (m_left_size == 3)
		{
			EncodePart(m_left, 3, out);
			m_left_size = 0;
		}
	}

	while (i + 3 <= size)
	{
		EncodePart(&data[i], 3, out);
		i += 3;
	}

	while (i < size && m_left_size < sizeof(m_left) / sizeof(m_left[0]))
	{
		m_left[m_left_size++] = data[i++];
	}
	
}

void Base64StreamEncoder::Finalize(std::ostream& out)
{
	if (m_left_size > 0)
	{
		EncodePart(m_left, m_left_size, out);
		m_left_size = 0;
	}

	if (m_line_length > 0)
		out << "\r\n";
}