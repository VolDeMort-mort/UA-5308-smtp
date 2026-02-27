#include "SocketConnection.hpp"

using boost::asio::ip::tcp;

SocketConnection::SocketConnection(tcp::socket socket) : m_socket(std::move(socket)), m_connected(m_socket.is_open()) {}

bool SocketConnection::send(const std::string& data)
{
	if (!m_connected) return false;

	boost::system::error_code ec;

	boost::asio::write(m_socket, boost::asio::buffer(data), ec);

	if (ec)
	{
		m_connected = false;
		return false;
	}

	return true;
}

bool SocketConnection::receive(std::string& outData)
{
	if (!m_connected) return false;

	boost::asio::streambuf buffer;
	boost::system::error_code ec;

	boost::asio::read_until(m_socket, buffer, "\r\n", ec);

	if (ec)
	{
		m_connected = false;
		return false;
	}

	std::istream input(&buffer);
	std::getline(input, outData);

	if (!outData.empty() && outData.back() == '\r') outData.pop_back();

	return true;
}

bool SocketConnection::close()
{
	if (!m_connected) return false;

	boost::system::error_code ec;

	m_socket.shutdown(tcp::socket::shutdown_both, ec);
	m_socket.close(ec);

	m_connected = false;
	return true;
}

bool SocketConnection::isConnected() const noexcept
{
	return m_connected;
}
