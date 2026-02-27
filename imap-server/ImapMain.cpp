#include <boost/asio.hpp>

#include "ImapServer.hpp"

int main(int argc, char** argv)
{
	boost::asio::io_context io;
	Imap imap(io);
	imap.Start();
	return 0;
}
