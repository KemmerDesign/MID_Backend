#pragma once
#include <drogon/HttpController.h>
#include "../models/Supplier.h"

using namespace drogon;

class SupplierController : public drogon::HttpController<SupplierController>
{
public:
    METHOD_LIST_BEGIN
        // Ruta para crear (POST)
        ADD_METHOD_TO(SupplierController::createSupplier, "/api/suppliers", Post);
        // Ruta para listar (GET)
        ADD_METHOD_TO(SupplierController::getSuppliers, "/api/suppliers", Get);
    METHOD_LIST_END
    void createSupplier(const HttpRequestPtr &req,
                        std::function<void (const HttpResponsePtr &)> &&callback);

    void getSuppliers(const HttpRequestPtr &req,
                      std::function<void (const HttpResponsePtr &)> &&callback);
};