#include "ApiHandler.h"
 
#include <boost/json.hpp>
#include <iostream>
 
namespace json = boost::json;
namespace http = boost::beast::http;

ApiHandler::ApiHandler(UserRepository& user_repo, ILogger& logger)  : m_user_repo(user_repo), m_logger(logger) {}

http::response<http::string_body> ApiHandler::handleRegister(const std::string& body)
{
    json::value parsed;

    try{
        parsed = json::parse(body);
    }
    catch(...){
        return errorResponse(http::status::bad_request, "Invalid JSON");
    }

    json::object* obj = parsed.if_object();
    if (!obj){
        return errorResponse(http::status::bad_request, "Expected JSON object");
    }

    json::value* username_val = obj->if_contains("username");
    json::value* password_val = obj->if_contains("password");

    if(!username_val || !password_val){
        return errorResponse(http::status::bad_request, "Missing 'username' or 'password'");
    }

    json::string* username_str = username_val->if_string();
    json::string* password_str = password_val->if_string();
 
    if (!username_str || !password_str){
        return errorResponse(http::status::bad_request, "'username' and 'password' must be strings");
    }
 
    std::string username(username_str->c_str());
    std::string password(password_str->c_str());
 
    if (username.empty() || password.empty()){
        return errorResponse(http::status::bad_request, "'username' and 'password' must not be empty");
    }
 
    User user;
    user.username = username;
 
    if (!m_user_repo.registerUser(user, password)){
        return errorResponse(http::status::conflict, m_user_repo.getLastError());
    }
 
    return okResponse("User registered successfully");
}

http::response<http::string_body> ApiHandler::handleChangePassword(const std::string& body)
{
    json::value parsed;

    try{
        parsed = json::parse(body);
    }
    catch(...){
        return errorResponse(http::status::bad_request, "Invalid JSON");
    }

    json::object* obj = parsed.if_object();
    if(!obj){
        return errorResponse(http::status::bad_request, "Expected JSON object");
    }

    json::value* username_val = obj->if_contains("username");
    json::value* old_password_val = obj->if_contains("old_password");
    json::value* new_password_val = obj->if_contains("new_password");
 
    if (!username_val || !old_password_val || !new_password_val){
        return errorResponse(http::status::bad_request, "Missing 'username', 'old_password' or 'new_password'");
    }
 
    json::string* username_str = username_val->if_string();
    json::string* old_password_str = old_password_val->if_string();
    json::string* new_password_str = new_password_val->if_string();
 
    if (!username_str || !old_password_str || !new_password_str){
        return errorResponse(http::status::bad_request, "All fields must be strings");
    }
 
    std::string username(username_str->c_str());
    std::string old_password(old_password_str->c_str());
    std::string new_password(new_password_str->c_str());
 
    if (username.empty() || old_password.empty() || new_password.empty()){
        return errorResponse(http::status::bad_request, "Fields must not be empty");
    }
 
    if (!m_user_repo.authorize(username, old_password)){
        return errorResponse(http::status::unauthorized, "Invalid credentials");
    }
 
    std::optional<User> user = m_user_repo.findByUsername(username);
    if (!user || !user->id){
        return errorResponse(http::status::not_found, "User not found");
    }
 
    if (!m_user_repo.changePassword(user->id.value(), new_password)){
        return errorResponse(http::status::internal_server_error, m_user_repo.getLastError());
    }
 
    return okResponse("Password changed successfully");
}

http::response<http::string_body> ApiHandler::okResponse(const std::string& message)
{
    json::object obj;
    obj["status"] = "ok";
    obj["message"] = message;

    const int http_version = 11;
    http::response<http::string_body> response{http::status::ok, http_version};
    response.set(http::field::content_type, "application/json");
    response.body() = json::serialize(obj);
    response.prepare_payload();
	return response;
}

http::response<http::string_body> ApiHandler::errorResponse(http::status status, const std::string& message)
{
    json::object obj;
    obj["status"] = "error";
    obj["message"] = message;

    const int http_version = 11;
    http::response<http::string_body> response{status, http_version};
    response.set(http::field::content_type, "application/json");
    response.body() = json::serialize(obj);
    response.prepare_payload();
	return response;
}

http::response<http::string_body> ApiHandler::handle(const http::request<http::string_body>& request)
{
    if(request.method() == http::verb::post && request.target() == "/register"){
        return handleRegister(request.body());
    }
    if(request.method() == http::verb::post && request.target() == "/change-password"){
        return handleChangePassword(request.body());
    }

	return errorResponse(http::status::not_found, "Unknown endpoint");
}
