#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <functional>

#include "PlainStream.h"
#include "SMTPCommandParser.h"
#include "SMTP_Types.h"
#include "SslStream.h"

class SMTPServerSocket : public std::enable_shared_from_this<SMTPServerSocket>
{
public:
	using Ptr = std::shared_ptr<SMTPServerSocket>;
	using ReadHandler = std::function<void(ClientCommand, const boost::system::error_code&)>;
	using WriteHandler = std::function<void(const boost::system::error_code&)>;

	SMTPServerSocket(boost::asio::ip::tcp::socket socket);
	~SMTPServerSocket();

	void ReadCommand(std::chrono::milliseconds timeout, ReadHandler handler);
	void ReadDataBlock(std::chrono::milliseconds timeout, ReadHandler handler);

	void SendResponse(const ServerResponse& resp, WriteHandler handler);

	void StartTls(boost::asio::ssl::context& ctx, std::function<void(const boost::system::error_code&)> handler);

	bool IsTls() { return m_is_tls; }

	void Close();

private:
	std::unique_ptr<IStream> m_socket;
	boost::asio::steady_timer m_timer;
	boost::asio::strand<boost::asio::any_io_executor> m_strand;
	boost::asio::streambuf m_buf;

	bool m_closing = false;
	bool m_is_tls = false;
};
