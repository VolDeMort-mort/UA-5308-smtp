#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <deque>
#include <functional>
#include <memory>
#include <string>

#include "PlainStream.h"
#include "SMTP_Types.h"
#include "SslStream.h"

class SMTPClientSocket : public std::enable_shared_from_this<SMTPClientSocket>
{
public:
	using Ptr = std::shared_ptr<SMTPClientSocket>;
	using ReadHandler = std::function<void(ServerResponse, const boost::system::error_code& ec)>;
	using WriteHandler = std::function<void(const boost::system::error_code& ec)>;

	SMTPClientSocket(boost::asio::io_context& ioc, std::size_t max_smtp_line = MAX_SMTP_LINE);
	~SMTPClientSocket();

	void Connect(const boost::asio::ip::tcp::endpoint& ep,
				 std::function<void(const boost::system::error_code&)> handler);

	void ReadResponse(std::chrono::milliseconds timeout, ReadHandler handler);

	void SendCommand(const ClientCommand& cmd, WriteHandler handler);

	void Close();

	void StartTls(boost::asio::ssl::context& ctx, std::function<void(const boost::system::error_code&)> handler);

	bool IsTls() const { return m_is_tls; }

private:
	std::unique_ptr<IStream> m_socket;
	boost::asio::steady_timer m_timer;
	boost::asio::strand<boost::asio::any_io_executor> m_strand;
	std::string m_buf;

	const size_t M_MAX_SMTP_LINE;

	bool m_closing = false;
	bool m_is_tls = false;

	void CreateCommandString(const ClientCommand& cmd, std::string& out);
};
