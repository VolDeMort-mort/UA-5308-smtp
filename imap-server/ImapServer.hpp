#pragma once

#include <boost/asio.hpp>

#include "../logger/Logger.h"

constexpr int IMAP_PORT = 2553;

class Imap
{
public:
	Imap(boost::asio::io_context& context, Logger& m_logger);
	Imap(const Imap&) = delete;
	void Start();

private:
	void AcceptConnection();

	boost::asio::io_context& m_context;
	boost::asio::ip::tcp::acceptor m_acceptor;
	Logger& m_logger;
};
