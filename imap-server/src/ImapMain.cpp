#include <boost/asio.hpp>

#include "FileStrategy.h"
#include "ImapServer.hpp"
#include "Logger.h"

int main(int argc, char** argv)
{
	Logger logger(std::make_unique<FileStrategy>(PROD));
	logger.Log(DEBUG, "Imap main started");

	boost::asio::io_context io;
	DataBaseManager db("test.db", "../../database/scheme/001_init_scheme.sql");

	Imap imap(io, logger, db);
	imap.Start();
	logger.Log(DEBUG, "Imap main ended");
	return 0;
}
