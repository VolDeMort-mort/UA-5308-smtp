#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <sstream>

#include "ApiServer.h"
#include "DataBaseManager.h"
#include "UserDAL.h"
#include "UserRepository.h"
#include "ThreadPool.h"
#include "Logger.h"
#include "FileStrategy.h"

int main()
{
    constexpr uint16_t API_PORT = 8080;
    constexpr int WORKER_THREADS = 4;

    auto logger = std::make_shared<Logger>(std::make_unique<FileStrategy>(LogLevel::PROD));

    std::ifstream sql_file("../database/scheme/001_init_scheme.sql");
    if (!sql_file.is_open())
    {
        std::cerr << "[ApiServer] Failed to open migration file\n";
        return 1;
    }
    std::stringstream ss;
    ss << sql_file.rdbuf();
    std::string migration_sql = ss.str();

    DataBaseManager db("prod.db", migration_sql, logger, 4);
    if (!db.isConnected())
    {
        logger->Log(LogLevel::PROD, "[ApiServer] Database connection failed");
        return 1;
    }

    UserRepository user_repo(db);

    ThreadPool pool;
    pool.initialize(WORKER_THREADS);

    ApiServer server(pool, user_repo, *logger, API_PORT);
    server.start();

    logger->Log(LogLevel::PROD, "[ApiServer] running on port " + std::to_string(API_PORT));
    logger->Log(LogLevel::PROD, "[ApiServer] press Enter to stop");
    std::cin.get();

    server.stop();
    return 0;
}