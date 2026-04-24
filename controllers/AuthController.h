#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class AuthController : public drogon::HttpController<AuthController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthController::login,    "/api/v1/auth/login",    Post);
    ADD_METHOD_TO(AuthController::register_, "/api/v1/auth/register", Post);
    METHOD_LIST_END

    void login(const HttpRequestPtr& req,
               std::function<void(const HttpResponsePtr&)>&& callback);

    void register_(const HttpRequestPtr& req,
                   std::function<void(const HttpResponsePtr&)>&& callback);
};
