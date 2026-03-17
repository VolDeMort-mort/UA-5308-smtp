#include <boost/asio.hpp>
#include <exception>
#include <mini/ini.h>

#include "FileStrategy.h"
#include "ImapConfig.hpp"
#include "ImapServer.hpp"
#include "Logger.h"

int main(int argc, char** argv)
{
	Logger logger(std::make_unique<FileStrategy>(TRACE));
	logger.Log(TRACE, "IMAP main started");

	// assuming you are running from default build location (/build/imap-server/)
	mINI::INIFile file("../../imap-server/imap.config");
	mINI::INIStructure ini;
	ImapConfig config;

	if (file.read(ini))
	{
		logger.Log(PROD, "Config read successfully");
		if (config.Parse(ini, logger))
		{
			logger.Log(PROD, "Config parsed successfully");
		}
		else
		{
			logger.Log(PROD, "Config parsed unsuccessfully/partly successfully");
		}
	}
	else
	{
		logger.Log(PROD, "Couldn`t read config. Terminating the program");
		return 1;
	}

	try
	{
		boost::asio::io_context io;
		// assuming you are running from default build location (/build/imap-server/)
		DataBaseManager db("prod.db", "../../database/scheme/001_init_scheme.sql");
		ThreadPool pool;
		pool.initialize(config.WORKER_THREADS);
		pool.set_logger(&logger);

		ImapServer imap(io, logger, db, pool, config);
		imap.Start();
	}
	catch (const std::exception& e)
	{
		logger.Log(PROD, std::string("Couldn`t create IMAP server entity: ") + e.what());
	}

	logger.Log(TRACE, "IMAP main ended");
	return 0;
}
