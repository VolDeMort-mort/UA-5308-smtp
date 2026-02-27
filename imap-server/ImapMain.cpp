#include <boost/asio.hpp>

#include "../logger/Logger.h"
#include "../logger/FileStrategy.h"
#include "ImapServer.hpp"

int main(int argc, char** argv)
{
	Logger logger(std::make_unique<FileStrategy>(PROD));
	logger.Log(DEBUG, "Imap main started");
	boost::asio::io_context io;
	Imap imap(io, logger);
	imap.Start();
	logger.Log(DEBUG, "Imap main ended");
	return 0;
}
