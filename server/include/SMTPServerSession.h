#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "SMTPServerSocket.h"
#include "SMTP_Types.h"

class SMTPServerSession : public std::enable_shared_from_this<SMTPServerSession>
{
public:
	using Ptr = std::shared_ptr<SMTPServerSession>;

	SMTPServerSession(SMTPServerSocket::Ptr socket, std::shared_ptr<boost::asio::ssl::context> ssl_ctx);

	void run();

private:
	enum class State
	{
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

	const std::chrono::seconds M_CMD_TIMEOUT{std::chrono::seconds(300)};
	const std::chrono::seconds M_DATA_TIMEOUT{std::chrono::seconds(600)};

	void SendResponse(const ServerResponse& resp);
	void DoReadCommand();
	void HandleCommand(ClientCommand cmd, const boost::system::error_code& ec);
	void StartReadData();

	static bool ExtractAddress(std::string& s);
};
