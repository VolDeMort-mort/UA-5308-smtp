#include "SocketAcceptor.hpp"

using boost::asio::ip::tcp;

bool SocketAcceptor::initialize(boost::asio::io_context& ioContext,
                                uint16_t port)
{
    try
    {
        tcp::endpoint endpoint(tcp::v4(), port);

        m_acceptor = std::make_unique<tcp::acceptor>(ioContext);
        m_acceptor->open(endpoint.protocol());
        m_acceptor->set_option(tcp::acceptor::reuse_address(true));
        m_acceptor->bind(endpoint);
        m_acceptor->listen();

        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool SocketAcceptor::accept(
    std::unique_ptr<SocketConnection>& outConnection)
{
    if (!m_acceptor)
        return false;

    boost::asio::ip::tcp::socket socket(
        m_acceptor->get_executor());

    boost::system::error_code ec;
    m_acceptor->accept(socket, ec);

    if (ec)
        return false;

    outConnection =
        std::make_unique<SocketConnection>(std::move(socket));

    return true;
}

bool SocketAcceptor::stop()
{
    if (!m_acceptor)
        return false;

    boost::system::error_code ec;
    m_acceptor->close(ec);
    return !ec;
}

