#pragma once

#include "IStream.h"

class SslStream : public IStream
{
public:
	explicit SslStream(boost::asio::ssl::stream<boost::asio::ip::tcp::socket>&& s) : m_ssl_stream(std::move(s)) {}

	boost::asio::any_io_executor get_executor() override { return m_ssl_stream.get_executor(); }

	void async_read_until(std::string& buf, std::size_t max_size, const std::string& delim,
						  std::function<void(const boost::system::error_code&, std::size_t)> handler) override
	{
		boost::asio::async_read_until(m_ssl_stream, boost::asio::dynamic_buffer(buf, max_size), delim,
									  std::move(handler));
	}

	void async_write(const boost::asio::const_buffer& buf,
					 std::function<void(const boost::system::error_code&, std::size_t)> handler) override
	{
		boost::asio::async_write(m_ssl_stream, boost::asio::buffer(buf), std::move(handler));
	}

	void shutdown() override
	{
		boost::system::error_code ec;

		m_ssl_stream.shutdown(ec);
		m_ssl_stream.lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		m_ssl_stream.lowest_layer().close(ec);
	}

	boost::asio::ip::tcp::socket release_tcp_socket() override { return std::move(m_ssl_stream.next_layer()); }

	void cancel() override
	{
		boost::system::error_code ec;
		m_ssl_stream.lowest_layer().cancel(ec);
	}

private:
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket> m_ssl_stream;
};
