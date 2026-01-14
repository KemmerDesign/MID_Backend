#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class SupplierController : public drogon::HttpController<SupplierController> {
public:
    METHOD_LIST_BEGIN
    // Registramos la ruta: POST /api/suppliers
    ADD_METHOD_TO(SupplierController::createOne, "/api/suppliers", Post);
    ADD_METHOD_TO(SupplierController::getOne, "/api/suppliers/{1}", Get);
    METHOD_LIST_END

    void createOne(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback);
    
    // Otros métodos...
    void getOne(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, std::string &&id);
};