#pragma once
#include <ostream>
#include "Base64Common.hpp"

class Base64StreamDecoder
{
public:
	Base64StreamDecoder() : m_buffer{}, m_buffer_size(0) {};

	void DecodeStream(const char* data, std::size_t size, std::ostream& out);
	void Finalize(std::ostream& out);

private:
	char m_buffer[4];
	std::size_t m_buffer_size;

	void DecodePart(const char* part, std::ostream& out);
};