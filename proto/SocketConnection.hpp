#pragma once

#include <boost/asio.hpp>
#include <string>

class SocketConnection
{
public:
    explicit SocketConnection(boost::asio::ip::tcp::socket socket);

    bool Send(const std::string& data);
	bool SendRaw(const unsigned char* buffer, std::size_t size);
    bool Receive(std::string& out_data);
	bool ReceiveRaw(unsigned char* buffer, std::size_t size);
    bool Close();
    bool IsOpen() const noexcept;

    void SetTimeout(int seconds);

private:
    static constexpr size_t MAX_LINE_SIZE = 1024 * 1024;

    boost::asio::ip::tcp::socket m_socket;
    boost::asio::streambuf m_buffer;

    int m_timeout_seconds{30};
	bool WaitForEvent(bool for_read);
};
