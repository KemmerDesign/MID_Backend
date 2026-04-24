#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class CommercialController : public drogon::HttpController<CommercialController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(CommercialController::createClient, "/api/v1/commercial/clients",     Post);
    ADD_METHOD_TO(CommercialController::listClients,  "/api/v1/commercial/clients",     Get);
    ADD_METHOD_TO(CommercialController::getClient,    "/api/v1/commercial/clients/{1}", Get);
    ADD_METHOD_TO(CommercialController::createVisit,  "/api/v1/commercial/visits",      Post);
    METHOD_LIST_END

    void createClient(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void listClients (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void getClient   (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void createVisit (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
};
