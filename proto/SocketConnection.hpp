#pragma once

#include <boost/asio.hpp>
#include <string>

class SocketConnection
{
public:
	explicit SocketConnection(boost::asio::ip::tcp::socket socket);

	bool send(const std::string& data);
	bool sendRaw(const unsigned char* buffer, std::size_t size);
	bool receive(std::string& outData);
	bool receiveRaw(unsigned char* buffer, std::size_t size); 
	bool close();
	bool isConnected() const noexcept;

private:
	boost::asio::ip::tcp::socket m_socket;
	bool m_connected{false};
};
