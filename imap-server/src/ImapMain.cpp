#include <boost/asio.hpp>
#include <exception>
#include <mini/ini.h>

#include "FileStrategy.h"
#include "ImapConfig.hpp"
#include "ImapServer.hpp"
#include "Logger.h"
#include "UserRepository.h"

// use with relative path to root folder from directory you are launching from
int main(int argc, char** argv)
{
	Logger logger(std::make_unique<FileStrategy>(TRACE));
	logger.Log(TRACE, "IMAP main started");

	if (argc < 2)
	{
		logger.Log(PROD, "Wrong usage. Did`t get relative path to root folder");
		return 1;
	}

	const std::string path_to_root = argv[1];

	mINI::INIFile file(path_to_root + "./imap-server/imap.config");
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
		DataBaseManager db(path_to_root + "./data/mail.db", path_to_root + "./database/scheme/001_init_scheme.sql",
						   std::shared_ptr<ILogger>(&logger, [](ILogger*) {}));
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
