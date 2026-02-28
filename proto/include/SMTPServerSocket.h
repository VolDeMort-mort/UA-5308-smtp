#pragma once
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "SMTP_Types.h"
#include "SMTPCommandParser.h"
#include <functional>

#include "PlainStream.h"
#include "SslStream.h"

class SMTPServerSocket : public std::enable_shared_from_this<SMTPServerSocket>
{
public:
	using Ptr = std::shared_ptr<SMTPServerSocket>;
	using ReadHandler = std::function<void(ClientCommand, const boost::system::error_code&)>;
	using WriteHandler = std::function<void(const boost::system::error_code&)>;

	SMTPServerSocket(boost::asio::ip::tcp::socket socket);
	~SMTPServerSocket();

	void readCommand(std::chrono::milliseconds timeout, ReadHandler handler);
	void readDataBlock(std::chrono::milliseconds timeout, ReadHandler handler);

	void sendResponse(const ServerResponse& resp, WriteHandler handler);

	void SMTPServerSocket::start_tls(boost::asio::ssl::context& ctx,
									 std::function<void(const boost::system::error_code&)> handler);

	std::error_code last_error() const;

	void close();

private:
	std::unique_ptr<IStream> m_socket;
	boost::asio::steady_timer m_timer;
	boost::asio::strand<boost::asio::any_io_executor> m_strand;
	boost::asio::streambuf m_buf;

	std::error_code m_last_error;
	bool m_closing = false;
	bool m_is_tls = false;
};
