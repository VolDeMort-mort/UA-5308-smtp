#include <string>

#include <boost/asio.hpp>

#include "SocketConnector.hpp"

using boost::asio::ip::tcp;

bool SocketConnector::Initialize(boost::asio::io_context& io_context)
{
    m_io_context = &io_context;
    return true;
}

bool SocketConnector::Connect(const std::string& host,
                               uint16_t port,
                               std::unique_ptr<SocketConnection>& connection)
{
    if (!m_io_context)
        return false;

    try
    {
        tcp::resolver resolver(*m_io_context);
        auto endpoints = resolver.resolve(host, std::to_string(port));

        tcp::socket socket(*m_io_context);
        boost::asio::connect(socket, endpoints);

        connection = std::make_unique<SocketConnection>(std::move(socket));

        return true;
    }
    catch (...)
    {
        return false;
    }
}
