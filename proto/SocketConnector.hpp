#pragma once

#include <boost/asio.hpp>
#include <memory>

#include "SocketConnection.hpp"

class SocketConnector
{
public:
	SocketConnector() = default;

	bool initialize(boost::asio::io_context& ioContext);

	bool connect(const std::string& host, uint16_t port, std::unique_ptr<SocketConnection>& outConnection);

private:
	boost::asio::io_context* m_ioContext{nullptr};
};
