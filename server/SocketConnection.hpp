#pragma once

#include <boost/asio.hpp>
#include <string>

class SocketConnection
{
public:
    explicit SocketConnection(boost::asio::ip::tcp::socket socket);

    bool Send(const std::string& data);
    bool Receive(std::string& out_data);
    bool Close();
    bool IsOpen() const noexcept;

private:
    static constexpr size_t MAX_LINE_SIZE = 1000;

    boost::asio::ip::tcp::socket m_socket;
    boost::asio::streambuf m_buffer;
};
