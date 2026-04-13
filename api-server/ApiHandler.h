#pragma once

#include <boost/beast/http.hpp>
#include <string>

#include "UserRepository.h"
#include "ILogger.h"

namespace http = boost::beast::http;

class ApiHandler
{
private:
    http::response<http::string_body> handleRegister(const std::string& body);
    http::response<http::string_body> handleChangePassword(const std::string& body);

    http::response<http::string_body> okResponse(const std::string& message);
    http::response<http::string_body> errorResponse(http::status status, const std::string& message);

    UserRepository& m_user_repo;
    ILogger& m_logger;
public:
    explicit ApiHandler(UserRepository& user_repo, ILogger& logger);

    http::response<http::string_body> handle(const http::request<http::string_body>& request);
};