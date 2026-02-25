#pragma once
#include <ostream>
#include "Base64Common.hpp"
#include "AttachmentConfig.hpp"

class Base64StreamEncoder
{
public:
	Base64StreamEncoder() : m_left{}, m_left_size(0), m_line_length(0) {};

	void EncodeStream(const uint8_t* data, std::size_t size, std::ostream& out);
	void Finalize(std::ostream& out);

private:
	uint8_t m_left[3];
	std::size_t m_left_size;
	std::size_t m_line_length;

	void EncodePart(const uint8_t* data, std::size_t size, std::ostream& out);
};