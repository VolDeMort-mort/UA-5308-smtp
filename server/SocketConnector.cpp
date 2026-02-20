#include "SocketConnector.hpp"

using boost::asio::ip::tcp;

bool SocketConnector::initialize(boost::asio::io_context& ioContext)
{
	m_ioContext = &ioContext;
	return true;
}

bool SocketConnector::connect(const std::string& host, uint16_t port, std::unique_ptr<SocketConnection>& outConnection)
{
	if (!m_ioContext) return false;

	try
	{
		tcp::resolver resolver(*m_ioContext);
		auto endpoints = resolver.resolve(host, std::to_string(port));

		tcp::socket socket(*m_ioContext);
		boost::asio::connect(socket, endpoints);

		outConnection = std::make_unique<SocketConnection>(std::move(socket));

		return true;
	}
	catch (...)
	{
		return false;
	}
}
