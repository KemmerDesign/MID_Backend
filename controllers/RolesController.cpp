#include "RolesController.h"
#include "../models/rrhh/Role.h"
#include "../utils/JwtUtils.h"
#include <nlohmann/json.hpp>
#include <set>

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

static const std::set<std::string> VALID_DEPTS_ROLES = {
    "comercial","proyectos","produccion","almacen",
    "rrhh","gerencia","administrativo","financiero","tecnologia","otro"
};

// ── GET /api/v1/rrhh/roles ────────────────────────────────────────────────────
void RolesController::listRoles(const HttpRequestPtr& req,
                                 std::function<void(const HttpResponsePtr&)>&& cb) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    std::string dept = req->getParameter("departamento");
    try {
        auto roles = Role::findAll(claims->tenant_id, dept);
        json arr = json::array();
        for (const auto& ro : roles) arr.push_back(ro.toJson());
        cb(jsonResp(k200OK, {{"roles", arr}, {"total", arr.size()}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// ── POST /api/v1/rrhh/roles ───────────────────────────────────────────────────
void RolesController::createRole(const HttpRequestPtr& req,
                                  std::function<void(const HttpResponsePtr&)>&& cb) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    json body;
    try { body = json::parse(req->getBody()); }
    catch (...) { cb(jsonResp(k400BadRequest, {{"error","JSON mal formado"}})); return; }

    if (!body.contains("nombre") || body["nombre"].get<std::string>().empty())
        { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: nombre"}})); return; }
    if (!body.contains("departamento") || !VALID_DEPTS_ROLES.count(body["departamento"].get<std::string>()))
        { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: departamento (valor inválido)"}})); return; }

    try {
        auto ro = Role::create(claims->tenant_id, body);
        cb(jsonResp(k201Created, {{"role", ro.toJson()}}));
    } catch (const std::exception& e) {
        std::string msg = e.what();
        if (msg.find("uq_roles_nombre") != std::string::npos)
            cb(jsonResp(k409Conflict, pgErrBody(e, "Ya existe un rol con ese nombre en ese departamento")));
        else
            cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// ── GET /api/v1/rrhh/roles/{id} ───────────────────────────────────────────────
void RolesController::getRole(const HttpRequestPtr& req,
                               std::function<void(const HttpResponsePtr&)>&& cb,
                               std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        auto ro = Role::findById(id, claims->tenant_id);
        if (!ro) { cb(jsonResp(k404NotFound, {{"error","Rol no encontrado"}})); return; }
        cb(jsonResp(k200OK, {{"role", ro->toJson()}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// ── PATCH /api/v1/rrhh/roles/{id} ─────────────────────────────────────────────
void RolesController::updateRole(const HttpRequestPtr& req,
                                  std::function<void(const HttpResponsePtr&)>&& cb,
                                  std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    json body;
    try { body = json::parse(req->getBody()); }
    catch (...) { cb(jsonResp(k400BadRequest, {{"error","JSON mal formado"}})); return; }

    try {
        auto ro = Role::update(id, claims->tenant_id, body);
        if (!ro) { cb(jsonResp(k404NotFound, {{"error","Rol no encontrado"}})); return; }
        cb(jsonResp(k200OK, {{"role", ro->toJson()}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}
