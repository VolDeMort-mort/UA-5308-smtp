#include <gtest/gtest.h>

#include <boost/beast/http.hpp>
#include <boost/json.hpp>

#include <fstream>
#include <sstream>
#include <filesystem>

#include "ApiHandler.h"
#include "DataBaseManager.h"
#include "UserDAL.h"
#include "UserRepository.h"
#include "Logger.h"
#include "ConsoleStrategy.h"

namespace http = boost::beast::http;
namespace json = boost::json;

class ApiHandlerTest : public ::testing::Test
{
protected:
    static constexpr const char* SCHEMA_PATH = "../database/scheme/001_init_scheme.sql";

    void SetUp() override
    {
        std::filesystem::remove("test_api.db");
        
        std::ifstream sql_file(SCHEMA_PATH);
        ASSERT_TRUE(sql_file.is_open()) << "Cannot open schema: " << SCHEMA_PATH;
        std::stringstream ss;
        ss << sql_file.rdbuf();
        
        m_db = new DataBaseManager("test_api.db", ss.str());
        m_repo = new UserRepository(*m_db);
        m_logger = new Logger(std::make_unique<ConsoleStrategy>());
        m_handler = new ApiHandler(*m_repo, *m_logger);
    }

    void TearDown() override
    {
        delete m_handler;
        delete m_logger;
        delete m_repo;
        delete m_db;

        std::filesystem::remove("test_api.db");
    }

    http::request<http::string_body> makePost(const std::string& target, const std::string& body)
    {
        http::request<http::string_body> req{http::verb::post, target, 11};
        req.set(http::field::content_type, "application/json");
        req.body() = body;
        req.prepare_payload();
        return req;
    }

    json::object parseResponse(const http::response<http::string_body>& res)
    {
        return json::parse(res.body()).as_object();
    }

    DataBaseManager* m_db = nullptr;
    UserRepository* m_repo = nullptr;
    Logger* m_logger = nullptr;
    ApiHandler* m_handler = nullptr;
};

TEST_F(ApiHandlerTest, Register_Success)
{
    auto req = makePost("/register", R"({"username":"alice","password":"secret"})");
    auto res = m_handler->handle(req);

    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(parseResponse(res)["status"].as_string(), "ok");
}

TEST_F(ApiHandlerTest, Register_DuplicateUsername)
{
    auto req = makePost("/register", R"({"username":"alice","password":"secret"})");
    m_handler->handle(req);

    auto res = m_handler->handle(req);
    EXPECT_EQ(res.result(), http::status::conflict);
    EXPECT_EQ(parseResponse(res)["status"].as_string(), "error");
}

TEST_F(ApiHandlerTest, Register_MissingUsername)
{
    auto req = makePost("/register", R"({"password":"secret"})");
    auto res = m_handler->handle(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(ApiHandlerTest, Register_MissingPassword)
{
    auto req = makePost("/register", R"({"username":"alice"})");
    auto res = m_handler->handle(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(ApiHandlerTest, Register_EmptyUsername)
{
    auto req = makePost("/register", R"({"username":"","password":"secret"})");
    auto res = m_handler->handle(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(ApiHandlerTest, Register_EmptyPassword)
{
    auto req = makePost("/register", R"({"username":"alice","password":""})");
    auto res = m_handler->handle(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(ApiHandlerTest, Register_InvalidJson)
{
    auto req = makePost("/register", "not json at all");
    auto res = m_handler->handle(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(ApiHandlerTest, ChangePassword_Success)
{
    m_handler->handle(makePost("/register", R"({"username":"alice","password":"secret"})"));

    auto req = makePost("/change-password", R"({"username":"alice","old_password":"secret","new_password":"newpass"})");
    auto res = m_handler->handle(req);

    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(parseResponse(res)["status"].as_string(), "ok");
}

TEST_F(ApiHandlerTest, ChangePassword_WrongOldPassword_ReturnsUnauthorized)
{
    m_handler->handle(makePost("/register", R"({"username":"alice","password":"secret"})"));

    auto req = makePost("/change-password", R"({"username":"alice","old_password":"wrong","new_password":"newpass"})");
    auto res = m_handler->handle(req);

    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(ApiHandlerTest, ChangePassword_UnknownUser_ReturnsUnauthorized)
{
    auto req = makePost("/change-password", R"({"username":"nobody","old_password":"secret","new_password":"newpass"})");
    auto res = m_handler->handle(req);
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(ApiHandlerTest, ChangePassword_MissingFields_ReturnsBadRequest)
{
    auto req = makePost("/change-password", R"({"username":"alice"})");
    auto res = m_handler->handle(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(ApiHandlerTest, ChangePassword_InvalidJson_ReturnsBadRequest)
{
    auto req = makePost("/change-password", "not json");
    auto res = m_handler->handle(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(ApiHandlerTest, ChangePassword_NewPasswordWorksForLogin)
{
    m_handler->handle(makePost("/register", R"({"username":"alice","password":"secret"})"));

    m_handler->handle(makePost("/change-password", R"({"username":"alice","old_password":"secret","new_password":"newpass"})"));

    EXPECT_TRUE(m_repo->authorize("alice", "newpass"));
    EXPECT_FALSE(m_repo->authorize("alice", "secret"));
}

TEST_F(ApiHandlerTest, UnknownEndpoint_ReturnsNotFound)
{
    http::request<http::string_body> req{http::verb::get, "/unknown", 11};
    auto res = m_handler->handle(req);
    EXPECT_EQ(res.result(), http::status::not_found);
}

TEST_F(ApiHandlerTest, Response_HasJsonContentType)
{
    auto req = makePost("/register", R"({"username":"alice","password":"secret"})");
    auto res = m_handler->handle(req);
    EXPECT_EQ(res[http::field::content_type], "application/json");
}