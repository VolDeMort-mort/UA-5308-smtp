#include "ImapServer.hpp"

#include "ImapSession.hpp"

Imap::Imap(boost::asio::io_context& context)
	: m_context(context), m_acceptor(context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), IMAP_PORT))
{
}

void Imap::Start()
{
	AcceptConnection();
	m_context.run();
}

void Imap::AcceptConnection()
{
	m_acceptor.async_accept(
		[this](std::error_code ec, boost::asio::ip::tcp::socket socket)
		{
			if (!ec)
			{
				std::make_shared<ImapSession>(std::move(socket))->Start();
			}
			AcceptConnection();
		});
}
