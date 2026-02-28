#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <string>

#include "SocketConnection.hpp"

class SocketConnector
{
public:
    SocketConnector() = default;

    bool Initialize(boost::asio::io_context& io_context);

    bool Connect(const std::string& host,
                 uint16_t port,
                 std::unique_ptr<SocketConnection>& connection);

private:
    boost::asio::io_context* m_io_context{nullptr};
};
