#pragma once

#include <string>

class IConnection
{
public:
	virtual ~IConnection() = default;

	virtual bool send(const std::string& data) = 0;
	virtual bool receive(std::string& outData) = 0;
	virtual bool close() = 0;
	virtual bool isConnected() const noexcept = 0;
};
