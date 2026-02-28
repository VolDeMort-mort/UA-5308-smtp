#pragma once

#include <boost/asio.hpp>
#include <memory>

#include "SocketConnection.hpp"

class SocketAcceptor
{
public:
    SocketAcceptor() = default;

    bool Initialize(boost::asio::io_context& io_context,
                    uint16_t port);

    bool Accept(std::unique_ptr<SocketConnection>& connection);

    bool Stop();

private:
    std::unique_ptr<boost::asio::ip::tcp::acceptor> m_acceptor;
};
