#pragma once
#include <boost/asio.hpp>

#define IMAP_PORT 2553

class IMAP
{
public:
	IMAP(/*Logger logger, Repo rp,*/ boost::asio::io_context& context);
	IMAP(const IMAP&) = delete;
	void start();

private:
	// Logger logger_;
	// Repo rp_;
	boost::asio::io_context& context_;
	boost::asio::ip::tcp::acceptor acceptor_;
	void acceptConnection();
};