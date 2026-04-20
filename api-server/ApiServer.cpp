#include "ApiServer.h"
#include "ApiHandler.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>

ApiServer::ApiServer(ThreadPool& pool, UserRepository& user_repo, ILogger& logger, uint16_t port,
    const std::string& cert_file, const std::string& key_file) : m_pool(pool), m_user_repo(user_repo), m_logger(logger),
    m_port(port), m_cert_file(cert_file), m_key_file(key_file){}

void ApiServer::start()
{
    m_logger.Log(PROD, "ApiServer::start - Starting on port " + std::to_string(m_port));
    m_running = true;
    m_thread = std::thread(&ApiServer::run, this);
}
 
void ApiServer::stop()
{
    m_logger.Log(PROD, "ApiServer::stop - Stopping");

    m_running = false;

    if (m_acceptor)
    {
        boost::system::error_code ec;
        m_acceptor->close(ec);
    }

    m_io_context.stop();

    if (m_thread.joinable())
    {
        m_thread.join();
    }

    m_pool.terminate();
}
 
void ApiServer::handleConnection(ssl::stream<tcp::socket>&& ssl_socket)
{
    try
    {
        m_logger.Log(DEBUG, "ApiServer::handleConnection - Start");
 
        ssl_socket.handshake(ssl::stream_base::server);
 
        beast::flat_buffer buffer;
        http::request<http::string_body> req;
        http::read(ssl_socket, buffer, req);
 
        m_logger.Log(TRACE, "ApiServer::handleConnection - In: target=" + std::string(req.target()));
 
        ApiHandler handler(m_user_repo, m_logger);
        http::response<http::string_body> response = handler.handle(req);
 
        http::write(ssl_socket, response);
 
        beast::error_code ec;
        ssl_socket.shutdown(ec);
 
        m_logger.Log(DEBUG, "ApiServer::handleConnection - End");
    }
    catch (const std::exception& e)
    {
        m_logger.Log(PROD, "ApiServer::handleConnection - Error: " + std::string(e.what()));
    }
}
 
void ApiServer::run()
{
    try
    {
        m_ssl_ctx = std::make_unique<ssl::context>(ssl::context::tls);
        m_ssl_ctx->use_certificate_chain_file(m_cert_file);
        m_ssl_ctx->use_private_key_file(m_key_file, ssl::context::pem);

        m_acceptor = std::make_unique<tcp::acceptor>(
            m_io_context,
            tcp::endpoint(tcp::v4(), m_port)
        );

        m_logger.Log(PROD, "ApiServer::run - Listening on port " + std::to_string(m_port));

        while (m_running)
        {
            ssl::stream<tcp::socket> ssl_socket{m_io_context, *m_ssl_ctx};
            boost::system::error_code ec;

            m_acceptor->accept(ssl_socket.next_layer(), ec);

            if (ec)
            {
                if (!m_running)
                    break;

                m_logger.Log(PROD, "ApiServer::run - Accept error: " + ec.message());
                continue;
            }

            m_logger.Log(DEBUG, "ApiServer::run - Accepted new connection");

            m_pool.add_task([this, socket = std::move(ssl_socket)]() mutable
            {
                handleConnection(std::move(socket));
            });
        }
    }
    catch (const std::exception& e)
    {
        if (m_running)
        {
            m_logger.Log(PROD, "ApiServer::run - Error: " + std::string(e.what()));
        }
    }
}