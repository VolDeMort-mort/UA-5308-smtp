#pragma once

#include "IConnection.hpp"

#include <boost/asio.hpp>
#include <string>

class SocketConnection : public IConnection
{
public:
	explicit SocketConnection(boost::asio::ip::tcp::socket socket);

	bool send(const std::string& data) override;
	bool receive(std::string& outData) override;
	bool close() override;
	bool isConnected() const noexcept override;

	boost::asio::ip::tcp::socket releaseSocket();

private:
	boost::asio::ip::tcp::socket m_socket;
	bool m_connected{false};
};
