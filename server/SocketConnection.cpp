#include "SocketConnection.hpp"

using boost::asio::ip::tcp;

SocketConnection::SocketConnection(tcp::socket socket)
    : m_socket(std::move(socket))
{
}

bool SocketConnection::Send(const std::string& data)
{
    if (!m_socket.is_open())
        return false;

    if (data.empty())
        return false;

    boost::system::error_code error;

    boost::asio::write(m_socket,
                       boost::asio::buffer(data),
                       error);

    if (error)
    {
        Close();
        return false;
    }

    return true;
}

bool SocketConnection::Receive(std::string& out_data)
{
    if (!m_socket.is_open())
        return false;

    boost::system::error_code error;

    boost::asio::read_until(m_socket,
                            m_buffer,
                            "\r\n",
                            error);

    if (error == boost::asio::error::eof)
    {
        Close();
        return false;
    }

    if (error)
    {
        Close();
        return false;
    }

    if (m_buffer.size() > MAX_LINE_SIZE)
    {
        Close();
        return false;
    }

    std::istream stream(&m_buffer);
    std::getline(stream, out_data);

    if (!out_data.empty() && out_data.back() == '\r')
        out_data.pop_back();

    return true;
}

bool SocketConnection::Close()
{
    if (!m_socket.is_open())
        return false;

    boost::system::error_code error;

    m_socket.shutdown(tcp::socket::shutdown_both, error);
    m_socket.close(error);

    return true;
}

bool SocketConnection::IsOpen() const noexcept
{
    return m_socket.is_open();
}
