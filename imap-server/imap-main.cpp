#include <boost/asio.hpp>

#include "imap-server.hpp"

int main(int argc, char* argv)
{
	boost::asio::io_context io;
	IMAP imap(io);
	imap.start();
	return 0;
}