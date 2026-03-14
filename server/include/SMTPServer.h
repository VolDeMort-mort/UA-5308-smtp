#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

#include "SMTPServerSession.h"
#include "SMTPServerSocket.h"

class SMTPServer
{
public:
	SMTPServer(boost::asio::io_context& ioc, 
		unsigned short port, 
		const std::string& cert_file = "",
		const std::string& key_file = "", 
		std::chrono::seconds cmd_timeout = std::chrono::seconds(300),
		std::chrono::seconds data_timeout = std::chrono::seconds(600), 
		std::size_t max_smtp_line = MAX_SMTP_LINE,
		std::size_t max_data_size = MAX_DATA_SIZE);
	~SMTPServer();

	void run(std::size_t threads = 4);

	void Stop();

private:
	void DoAccept();

	boost::asio::io_context& m_ioc;
	boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_work_guard;
	boost::asio::ip::tcp::acceptor m_acceptor;

	const size_t M_MAX_SMTP_LINE;
	const size_t M_MAX_DATA_SIZE;

	const std::chrono::seconds M_CMD_TIMEOUT;
	const std::chrono::seconds M_DATA_TIMEOUT;

	std::atomic_bool m_running{false};
	std::vector<std::thread> m_threads;

	std::shared_ptr<boost::asio::ssl::context> m_ssl_ctx;
	bool m_has_tls{false};
};
