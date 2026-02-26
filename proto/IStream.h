#pragma once
#include <boost/asio.hpp>
#include <functional>
#include <memory>

class IStream
{
public:
	virtual ~IStream() = default;

	virtual void async_read_until(boost::asio::streambuf& buf, const std::string& delim,
								  std::function<void(const boost::system::error_code&, std::size_t)> handler) = 0;

	virtual void async_write(const boost::asio::const_buffer& buf,
							 std::function<void(const boost::system::error_code&, std::size_t)> handler) = 0;

	virtual void shutdown() = 0;

	virtual boost::asio::ip::tcp::socket release_tcp_socket() = 0;

	virtual void cancel() = 0;

	virtual boost::asio::any_io_executor get_executor() = 0;
};
