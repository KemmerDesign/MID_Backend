#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class ProductCatalogController : public drogon::HttpController<ProductCatalogController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ProductCatalogController::listCatalog,   "/api/v1/commercial/product-catalog",      Get);
    ADD_METHOD_TO(ProductCatalogController::createCatalog, "/api/v1/commercial/product-catalog",      Post);
    ADD_METHOD_TO(ProductCatalogController::getCatalog,    "/api/v1/commercial/product-catalog/{1}",  Get);
    ADD_METHOD_TO(ProductCatalogController::updateCatalog, "/api/v1/commercial/product-catalog/{1}",  Patch);
    METHOD_LIST_END

    void listCatalog  (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void createCatalog(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void getCatalog   (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void updateCatalog(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
};
