#pragma once
#include <boost/asio.hpp>
#include <functional>
#include <deque>
#include <memory>
#include <string>

#include "SMTP_Types.h"

class SMTPClientSocket : public std::enable_shared_from_this<SMTPClientSocket> {
public:
	using Ptr = std::shared_ptr<SMTPClientSocket>;
	using ReadHandler = std::function<void(ServerResponse, const boost::system::error_code& ec)>;
	using WriteHandler = std::function<void(const boost::system::error_code& ec)>;

	SMTPClientSocket(boost::asio::io_context& ioc);
	~SMTPClientSocket();

	void connect(const boost::asio::ip::tcp::endpoint& ep, std::function<void(const boost::system::error_code&)> handler);

	void readResponse(std::chrono::milliseconds timeout, ReadHandler handler);

	void sendCommand(const ClientCommand& cmd, WriteHandler handler);

	void close();

	std::error_code last_error() const;

private:

	boost::asio::ip::tcp::socket m_socket;
	boost::asio::steady_timer m_timer;
	boost::asio::strand<boost::asio::any_io_executor> m_strand;
	boost::asio::streambuf m_buf;

	std::error_code m_last_error;
	bool m_closing = false;

	void createCommandString(const ClientCommand& cmd, std::string& out);
};
