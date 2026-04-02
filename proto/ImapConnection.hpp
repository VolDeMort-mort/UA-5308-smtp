#pragma once

#include <boost/asio.hpp>

#include "IConnection.hpp"

class ImapConnection : public IConnection
{
public:
	explicit ImapConnection(boost::asio::ip::tcp::socket& socket) : m_socket(socket) {}

	bool Send(const std::string& data) override
	{
		boost::system::error_code ec;
		boost::asio::write(m_socket, boost::asio::buffer(data), ec);
		return !ec;
	}

	bool SendRaw(const unsigned char* buffer, std::size_t size) override
	{
		boost::system::error_code ec;
		boost::asio::write(m_socket, boost::asio::buffer(buffer, size), ec);
		return !ec;
	}

	bool Receive(std::string& out_data) override
	{
		boost::system::error_code ec;
		boost::asio::read_until(m_socket, m_buffer, "\r\n", ec);
		if (ec) return false;
		std::istream stream(&m_buffer);
		std::getline(stream, out_data);
		if (!out_data.empty() && out_data.back() == '\r') out_data.pop_back();
		return true;
	}

	bool ReceiveRaw(unsigned char* buffer, std::size_t size) override
	{
		boost::system::error_code ec;
		boost::asio::read(m_socket, boost::asio::buffer(buffer, size), ec);
		return !ec;
	}

	bool Close() override
	{
		boost::system::error_code ec;
		m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		m_socket.close(ec);
		return !ec;
	}

	bool IsOpen() const noexcept override { return m_socket.is_open(); }

private:
	boost::asio::ip::tcp::socket& m_socket;
	boost::asio::streambuf m_buffer;
};
