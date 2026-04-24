#include "ProductCatalogController.h"
#include "../models/commercial/ProductCatalog.h"
#include "../utils/JwtUtils.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static HttpResponsePtr jsonResp(HttpStatusCode code, const json& body) {
    auto resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(code);
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    resp->setBody(body.dump());
    return resp;
}

static json pgErrBody(const std::exception& e, const std::string& msg = "Error interno del servidor") {
    json body = {{"error", msg}};
    if (const auto* pe = dynamic_cast<const PgException*>(&e))
        if (!pe->sqlstate.empty()) body["pg_code"] = pe->sqlstate;
    return body;
}

// ── GET /api/v1/commercial/product-catalog ────────────────────────────────────
void ProductCatalogController::listCatalog(const HttpRequestPtr& req,
                                            std::function<void(const HttpResponsePtr&)>&& cb) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    bool only_active = req->getParameter("activo") != "false";

    try {
        auto items = ProductCatalog::findAll(claims->tenant_id, only_active);
        json arr = json::array();
        for (const auto& item : items) arr.push_back(item.toJson());
        cb(jsonResp(k200OK, {{"catalog", arr}, {"total", arr.size()}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// ── POST /api/v1/commercial/product-catalog ───────────────────────────────────
void ProductCatalogController::createCatalog(const HttpRequestPtr& req,
                                              std::function<void(const HttpResponsePtr&)>&& cb) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    json body;
    try { body = json::parse(req->getBody()); }
    catch (...) { cb(jsonResp(k400BadRequest, {{"error","JSON mal formado"}})); return; }

    if (!body.contains("nombre") || body["nombre"].get<std::string>().empty())
        { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: nombre"}})); return; }
    if (!body.contains("categoria") || body["categoria"].get<std::string>().empty())
        { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: categoria"}})); return; }

    try {
        auto item = ProductCatalog::create(claims->tenant_id, body);
        cb(jsonResp(k201Created, {{"catalog_item", item.toJson()}}));
    } catch (const std::exception& e) {
        std::string msg = e.what();
        if (msg.find("uq_product_catalog") != std::string::npos)
            cb(jsonResp(k409Conflict, pgErrBody(e, "Ya existe un producto con ese nombre en el catálogo")));
        else
            cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// ── GET /api/v1/commercial/product-catalog/{id} ───────────────────────────────
void ProductCatalogController::getCatalog(const HttpRequestPtr& req,
                                           std::function<void(const HttpResponsePtr&)>&& cb,
                                           std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        auto item = ProductCatalog::findById(id, claims->tenant_id);
        if (!item) { cb(jsonResp(k404NotFound, {{"error","Producto no encontrado"}})); return; }
        cb(jsonResp(k200OK, {{"catalog_item", item->toJson()}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// ── PATCH /api/v1/commercial/product-catalog/{id} ─────────────────────────────
void ProductCatalogController::updateCatalog(const HttpRequestPtr& req,
                                              std::function<void(const HttpResponsePtr&)>&& cb,
                                              std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    json body;
    try { body = json::parse(req->getBody()); }
    catch (...) { cb(jsonResp(k400BadRequest, {{"error","JSON mal formado"}})); return; }

    try {
        auto item = ProductCatalog::update(id, claims->tenant_id, body);
        if (!item) { cb(jsonResp(k404NotFound, {{"error","Producto no encontrado"}})); return; }
        cb(jsonResp(k200OK, {{"catalog_item", item->toJson()}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}
