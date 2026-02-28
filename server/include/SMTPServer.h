#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <thread>
#include <vector>
#include <optional>

#include "SMTPServerSocket.h"
#include "SMTPServerSession.h"

class SMTPServer {
public:
	SMTPServer(boost::asio::io_context& ioc, unsigned short port);
	~SMTPServer();

	void run(std::size_t threads = 1);

	void soft_stop();

private:
	void do_accept();

	boost::asio::io_context& m_ioc;
	boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_work_guard;
	boost::asio::ip::tcp::acceptor m_acceptor;
	std::atomic_bool m_running{ false };

	std::vector<std::thread> m_threads;
};
