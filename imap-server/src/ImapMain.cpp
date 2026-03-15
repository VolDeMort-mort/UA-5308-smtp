#include <boost/asio.hpp>
#include <exception>

#include "FileStrategy.h"
#include "ImapServer.hpp"
#include "Logger.h"

constexpr int THREADS_NUM = 4;

int main(int argc, char** argv)
{
	Logger logger(std::make_unique<FileStrategy>(TRACE));
	logger.Log(TRACE, "IMAP main started");

	try
	{
		boost::asio::io_context io;
		DataBaseManager db("prod.db", "../../database/scheme/001_init_scheme.sql");
		ThreadPool pool;
		pool.initialize(THREADS_NUM);
		pool.set_logger(&logger);

		ImapServer imap(io, logger, db, pool);
		imap.Start();
	}
	catch (const std::exception& e)
	{
		logger.Log(PROD, std::string("Couldn`t create IMAP server entity: ") + e.what());
	}

	logger.Log(TRACE, "IMAP main ended");
	return 0;
}
