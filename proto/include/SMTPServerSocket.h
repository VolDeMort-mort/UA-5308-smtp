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

	SMTPServerSocket(boost::asio::ip::tcp::socket socket, std::size_t max_smtp_line = MAX_SMTP_LINE,
					 std::size_t max_data_size = MAX_DATA_SIZE);
	~SMTPServerSocket();

	void ReadCommand(std::chrono::milliseconds timeout, ReadHandler handler);
	void ReadDataBlock(std::chrono::milliseconds timeout, ReadHandler handler);

	void SendResponse(const ServerResponse& resp, WriteHandler handler);

	void StartTls(boost::asio::ssl::context& ctx, std::function<void(const boost::system::error_code&)> handler);

	bool IsTls() const { return m_is_tls; }

	std::size_t getMaxDataSize() const { return M_MAX_DATA_SIZE; }

	void Close();

private:
	std::unique_ptr<IStream> m_socket;
	boost::asio::steady_timer m_timer;
	boost::asio::strand<boost::asio::any_io_executor> m_strand;

	std::string m_cmd_buf;
	std::string m_data_buf;

	const size_t M_MAX_SMTP_LINE;
	const size_t M_MAX_DATA_SIZE;

	bool m_closing = false;
	bool m_is_tls = false;
};
