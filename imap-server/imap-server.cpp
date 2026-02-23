#include "imap-server.hpp"

#include "imap-session.hpp"

IMAP::IMAP(/*Logger logger, Repo rp,*/ boost::asio::io_context& context)
	: context_(context),
	  acceptor_(context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), IMAP_PORT)) //, logger_(logger)
{
}

void IMAP::start()
{
	acceptConnection();
	context_.run();
}

void IMAP::acceptConnection()
{
	acceptor_.async_accept(
		[this](std::error_code ec, boost::asio::ip::tcp::socket socket)
		{
			if (!ec)
			{
				std::make_shared<IMAP_Session>(std::move(socket))->start();
			}
			acceptConnection();
		});
}