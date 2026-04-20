#include <gtest/gtest.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>

#include <atomic>
#include <thread>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>

#include "ApiServer.h"
#include "DataBaseManager.h"
#include "UserDAL.h"
#include "UserRepository.h"
#include "ThreadPool.h"
#include "Logger.h"
#include "ConsoleStrategy.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
namespace json = boost::json;
using tcp = net::ip::tcp;

static http::status sendRequest(const std::string& host, uint16_t port, const std::string& target, const std::string& body)
{
    try
    {
        net::io_context ioc;

        ssl::context ssl_ctx(ssl::context::tls_client);
        ssl_ctx.set_verify_mode(ssl::verify_none);

        ssl::stream<tcp::socket> stream{ioc, ssl_ctx};

        tcp::resolver resolver{ioc};
        auto endpoints = resolver.resolve(host, std::to_string(port));
        net::connect(stream.next_layer(), endpoints);

        stream.handshake(ssl::stream_base::client);

        http::request<http::string_body> req{http::verb::post, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::content_type, "application/json");
        req.body() = body;
        req.prepare_payload();

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.shutdown(ec);

        return res.result();
    }
    catch (...)
    {
        return http::status::internal_server_error;
    }
}

class ApiServerStressTest : public ::testing::Test
{
protected:
    static constexpr const char* SCHEMA_PATH = "../database/scheme/001_init_scheme.sql";

    void SetUp() override
    {
        std::ifstream sql_file(SCHEMA_PATH);
        ASSERT_TRUE(sql_file.is_open()) << "Cannot open schema: " << SCHEMA_PATH;
        std::stringstream ss;
        ss << sql_file.rdbuf();

        m_db = new DataBaseManager("test_api.db", ss.str());
        m_repo = new UserRepository(*m_db);
        m_logger = new Logger(std::make_unique<ConsoleStrategy>());

        m_pool = new ThreadPool();
        m_pool->initialize(8);

        m_server = new ApiServer(*m_pool, *m_repo, *m_logger, TEST_PORT, CERT_FILE, KEY_FILE);
        m_server->start();

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    void TearDown() override
    {
        m_server->stop();

        delete m_server;
        delete m_pool;
        delete m_logger;
        delete m_repo;
        delete m_db;

        std::filesystem::remove("test_api.db");
    }

    static constexpr uint16_t TEST_PORT = 8181;
    static constexpr const char* HOST = "localhost";
    static constexpr const char* CERT_FILE = "../certs/server.crt";
    static constexpr const char* KEY_FILE = "../certs/server.key";

    DataBaseManager* m_db = nullptr;
    UserRepository* m_repo = nullptr;
    Logger* m_logger = nullptr;
    ThreadPool* m_pool = nullptr;
    ApiServer* m_server = nullptr;
};

TEST_F(ApiServerStressTest, ConcurrentRegistrations_AllSucceed)
{
    constexpr int NUM_USERS = 50;

    std::atomic<int> success_count{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_USERS; ++i)
    {
        threads.emplace_back([&, i]()
        {
            std::string body = R"({"username":"user)" + std::to_string(i) + R"(","password":"pass)" + std::to_string(i) + R"("})";

            auto status = sendRequest(HOST, TEST_PORT, "/register", body);

            if (status == http::status::ok){
                ++success_count;
            }
        });
    }

    for (auto& t : threads){
        t.join();
    }

    EXPECT_EQ(success_count.load(), NUM_USERS);
}

TEST_F(ApiServerStressTest, ConcurrentDuplicateRegistrations_OnlyOneSucceeds)
{
    constexpr int NUM_THREADS = 20;

    std::atomic<int> success_count{0};
    std::vector<std::thread> threads;

    const std::string body = R"({"username":"alice","password":"secret"})";

    for (int i = 0; i < NUM_THREADS; ++i)
    {
        threads.emplace_back([&]()
        {
            auto status = sendRequest(HOST, TEST_PORT, "/register", body);
            if (status == http::status::ok){
                ++success_count;
            }
        });
    }

    for (auto& t : threads) t.join();

    EXPECT_EQ(success_count.load(), 1);
}

TEST_F(ApiServerStressTest, ConcurrentPasswordChanges_ServerStaysAlive)
{
    constexpr int NUM_THREADS = 30;

    for (int i = 0; i < NUM_THREADS; ++i)
    {
        std::string body = R"({"username":"user)" + std::to_string(i) + R"(","password":"oldpass"})";
        sendRequest(HOST, TEST_PORT, "/register", body);
    }

    std::atomic<int> success_count{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_THREADS; ++i)
    {
        threads.emplace_back([&, i]()
        {
            std::string body = R"({"username":"user)" + std::to_string(i) + R"(","old_password":"oldpass","new_password":"newpass"})";

            auto status = sendRequest(HOST, TEST_PORT, "/change-password", body);
            if (status == http::status::ok){
                ++success_count;
            }
        });
    }

    for (auto& t : threads){
        t.join();
    }

    EXPECT_EQ(success_count.load(), NUM_THREADS);
}

TEST_F(ApiServerStressTest, MixedRequests_ServerStaysAlive)
{
    constexpr int NUM_THREADS = 50;

    std::atomic<int> total_responses{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_THREADS; ++i)
    {
        threads.emplace_back([&, i]()
        {
            http::status status;

            if (i % 2 == 0)
            {
                std::string body = R"({"username":"mixed)" + std::to_string(i) + R"(","password":"pass"})";
                status = sendRequest(HOST, TEST_PORT, "/register", body);
            }
            else
            {
                std::string body = R"({"username":"nobody","old_password":"x","new_password":"y"})";
                status = sendRequest(HOST, TEST_PORT, "/change-password", body);
            }

            if (status != http::status::internal_server_error){
                ++total_responses;
            }
        });
    }

    for (auto& t : threads){ 
        t.join();
    }

    EXPECT_EQ(total_responses.load(), NUM_THREADS);
}