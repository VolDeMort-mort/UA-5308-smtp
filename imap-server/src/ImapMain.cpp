#include <boost/asio.hpp>
#include <sodium.h>

#include "FileStrategy.h"
#include "ImapServer.hpp"
#include "Logger.h"

int main(int argc, char** argv)
{
	Logger logger(std::make_unique<FileStrategy>(PROD));
	logger.Log(DEBUG, "Imap main started");

	if (sodium_init() < 0) 
	{
		logger.Log(DEBUG, "Sodium failed to init in Imap main");
		return 1;
	}

	boost::asio::io_context io;
	DataBaseManager db("prod.db", "../../database/scheme/001_init_scheme.sql");

	ImapServer imap(io, logger, db);
	imap.Start();
	logger.Log(DEBUG, "Imap main ended");
	return 0;
}
