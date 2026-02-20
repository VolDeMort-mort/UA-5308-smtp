#pragma once

#include <boost/asio.hpp>
#include <memory>

#include "SocketConnection.hpp"

class SocketAcceptor
{
public:
	SocketAcceptor() = default;

	bool initialize(boost::asio::io_context& ioContext, uint16_t port);

	bool accept(std::unique_ptr<SocketConnection>& outConnection);

	bool stop();

private:
	std::unique_ptr<boost::asio::ip::tcp::acceptor> m_acceptor;
};
