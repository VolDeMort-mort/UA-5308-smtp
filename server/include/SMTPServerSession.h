#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <chrono>

#include "SMTP_Types.h"
#include "SMTPServerSocket.h"

class SMTPServerSession : public std::enable_shared_from_this<SMTPServerSession> {
public:
	using Ptr = std::shared_ptr<SMTPServerSession>;

	SMTPServerSession(SMTPServerSocket::Ptr socket, std::shared_ptr<boost::asio::ssl::context> ssl_ctx);

	void run();

private:
	enum class State {
		NEW,
		READY,
		READING_DATA,
		CLOSED
	};

	SMTPServerSocket::Ptr m_socket;
	std::shared_ptr<boost::asio::ssl::context> m_ssl_ctx;
	State m_state;

	std::string m_mail_from;
	std::vector<std::string> m_recipients;
	std::string m_data_buffer;

	std::chrono::milliseconds m_cmd_timeout{ std::chrono::milliseconds(300000) };
	std::chrono::milliseconds m_data_timeout{ std::chrono::milliseconds(1200000) };

	void send_response(const ServerResponse& resp);
	void do_read_command();
	void handle_command(ClientCommand cmd, const boost::system::error_code& ec);
	void start_read_data();

	static bool extract_address(std::string& s);
};
