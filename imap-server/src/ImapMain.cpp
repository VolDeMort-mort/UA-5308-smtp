#include <boost/asio.hpp>
#include <exception>
#include <sodium.h>

#include "AppConfig.h"
#include "Config.h"
#include "FileStrategy.h"
#include "ImapServer.hpp"
#include "Logger.h"
#include "Repository/UserRepository.h"
#include "schema.h"

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

	if (SmtpClient::Config::Instance().Load(path_to_root + "./default_config.json"))
	{
		logger.Log(PROD, "Config read successfully");
	}
	else
	{
		logger.Log(PROD, "Couldn`t read config. Default values will be used");
	}

	try
	{
		if (sodium_init() < 0)
		{
			logger.Log(DEBUG, "Sodium failed to init in Imap main");
			return 1;
		}

		auto cnfg = SmtpClient::Config::Instance().GetImap();
		boost::asio::io_context io;
		DataBaseManager db(path_to_root + "./data/mail.db", initSchema(),
						   std::shared_ptr<ILogger>(&logger, [](ILogger*) {}));
		ThreadPool pool;
		pool.initialize(cnfg.worker_threads);
		pool.set_logger(&logger);

		ImapServer imap(io, logger, db, pool, cnfg);
		imap.Start();
	}
	catch (const std::exception& e)
	{
		logger.Log(PROD, std::string("Couldn`t create IMAP server entity: ") + e.what());
	}

	logger.Log(TRACE, "IMAP main ended");
	return 0;
}
