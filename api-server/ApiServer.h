#pragma once
 
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <cstdint>
#include <thread>

#include "UserRepository.h"
#include "ThreadPool.h"
#include "ILogger.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;

class ApiServer
{
private:
    ThreadPool& m_pool;
    UserRepository& m_user_repo;
    ILogger& m_logger;
    uint16_t m_port = 8080;
    boost::asio::io_context m_io_context;
    std::thread m_thread;
    std::string m_cert_file;
    std::string m_key_file;

    void run();
    void handleConnection(ssl::stream<tcp::socket>&&  ssl_socket);
public:
    explicit ApiServer(ThreadPool& pool, UserRepository& user_repo, ILogger& logger, uint16_t port, 
        const std::string& cert_file = "../certs/server.crt",
        const std::string& key_file = "../certs/server.key");

    void start();
    void stop();
};

