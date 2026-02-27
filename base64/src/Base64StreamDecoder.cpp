#include "Base64StreamDecoder.hpp"

void Base64StreamDecoder::DecodePart(const char* part, std::ostream& out)
{
    const auto& table = Base64::get_decode_table();

    int value0 = table[(unsigned char)part[0]];
    int value1 = table[(unsigned char)part[1]];
    int value2 = part[2] == '=' ? 0 : table[(unsigned char)part[2]];
    int value3 = part[3] == '=' ? 0 : table[(unsigned char)part[3]];

    if (value0 < 0 || value1 < 0 || value2 < -2 || value3 < -2)
    {
        throw std::runtime_error("Invalid Base64 character");
    }

    uint32_t block = (value0 << 18) | (value1 << 12) | (value2 << 6) | value3;

    out.put((block >> 16) & 0xFF);

    if (part[2] != '=')
    {
        out.put((block >> 8) & 0xFF);
    }
    if (part[3] != '=')
    {
        out.put(block & 0xFF);
    }
}

void Base64StreamDecoder::DecodeStream(const char* data, std::size_t size, std::ostream& out)
{
    for (std::size_t i = 0; i < size; ++i)
    {
        char c = data[i];

        if (c == '\r' || c == '\n' || c == ' ' || c == '\t')
            continue;

        m_buffer[m_buffer_size++] = c;

        if (m_buffer_size == 4)
        {
            DecodePart(m_buffer, out);
            m_buffer_size = 0;
        }
    }
}


void Base64StreamDecoder::Finalize(std::ostream& out)
{
    if (m_buffer_size != 0)
    {
        if (m_buffer_size == 4)
        {
			DecodePart(m_buffer, out);
        }
		else
		{
			throw std::runtime_error("Invalid Base64 stream");
        }
    }
}