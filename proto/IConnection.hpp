#pragma once
#include <string>
#include <cstddef>

class IConnection
{
public:
	virtual ~IConnection() = default;

	virtual bool Send(const std::string& data) = 0;
	virtual bool Receive(std::string& out_data) = 0;

	virtual bool SendRaw(const unsigned char* buffer, std::size_t size) = 0;
	virtual bool ReceiveRaw(unsigned char* buffer, std::size_t size) = 0;

	virtual bool IsOpen() const noexcept = 0;
	virtual bool Close() = 0;
};
