#pragma once
#include "IStream.h"

class PlainStream : public IStream, public std::enable_shared_from_this<PlainStream>
{
public:
	explicit PlainStream(boost::asio::ip::tcp::socket&& socket) : m_socket(std::move(socket)) {}

	boost::asio::any_io_executor get_executor() override { return m_socket.get_executor(); }

	void async_read_until(boost::asio::streambuf& buf, const std::string& delim,
						  std::function<void(const boost::system::error_code&, std::size_t)> handler) override
	{
		boost::asio::async_read_until(m_socket, buf, delim, std::move(handler));
	}

	void async_write(const boost::asio::const_buffer& buf,
					 std::function<void(const boost::system::error_code&, std::size_t)> handler) override
	{
		boost::asio::async_write(m_socket, boost::asio::buffer(buf), std::move(handler));
	}

	void shutdown() override
	{
		boost::system::error_code ec;
		m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		m_socket.close(ec);
	}

	boost::asio::ip::tcp::socket release_tcp_socket() override { 
		return std::move(m_socket); 
	}

	void cancel() override {
		boost::system::error_code ignored;
		m_socket.cancel(ignored);
	}


private:
	boost::asio::ip::tcp::socket m_socket;
};
