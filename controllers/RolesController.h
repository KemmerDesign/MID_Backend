#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class RolesController : public drogon::HttpController<RolesController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(RolesController::listRoles,   "/api/v1/rrhh/roles",      Get);
    ADD_METHOD_TO(RolesController::createRole,  "/api/v1/rrhh/roles",      Post);
    ADD_METHOD_TO(RolesController::getRole,     "/api/v1/rrhh/roles/{1}",  Get);
    ADD_METHOD_TO(RolesController::updateRole,  "/api/v1/rrhh/roles/{1}",  Patch);
    METHOD_LIST_END

    void listRoles  (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void createRole (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void getRole    (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void updateRole (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
};
