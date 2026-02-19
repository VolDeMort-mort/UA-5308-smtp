#pragma once

#include <boost/asio.hpp>
#include <string>

class SocketConnection
{
public:
    explicit SocketConnection(boost::asio::ip::tcp::socket socket);

    bool send(const std::string& data);
    bool receive(std::string& outData);
    bool close();
    bool isConnected() const noexcept;

private:
    boost::asio::ip::tcp::socket m_socket;
    bool m_connected{false};
};

