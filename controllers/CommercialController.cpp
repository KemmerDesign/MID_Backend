#include "CommercialController.h"
#include "../models/commercial/Client.h"
#include "../utils/JwtUtils.h"
#include "../utils/PgConnection.h"
#include <nlohmann/json.hpp>
#include <drogon/utils/Utilities.h>
#include <trantor/utils/Date.h>

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

// ── POST /api/v1/commercial/clients ──────────────────────────────────────────
void CommercialController::createClient(const HttpRequestPtr& req,
                                         std::function<void(const HttpResponsePtr&)>&& cb) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    json body;
    try { body = json::parse(req->getBody()); }
    catch (...) { cb(jsonResp(k400BadRequest, {{"error","JSON mal formado"}})); return; }

    if (!body.contains("nombre") || !body["nombre"].is_string() || body["nombre"].get<std::string>().empty())
        { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: nombre"}})); return; }
    if (!body.contains("nit") || !body["nit"].is_string() || body["nit"].get<std::string>().empty())
        { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: nit"}})); return; }
    if (!body.contains("direccion_principal") || body["direccion_principal"].get<std::string>().empty())
        { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: direccion_principal"}})); return; }

    try {
        auto client = Client::create(claims->tenant_id, body);
        cb(jsonResp(k201Created, {{"client", client.toJson()}}));
    } catch (const std::exception& e) {
        std::string msg = e.what();
        if (msg.find("clients_nit_tenant") != std::string::npos)
            cb(jsonResp(k409Conflict, pgErrBody(e, "Ya existe un cliente con ese NIT")));
        else
            cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// ── GET /api/v1/commercial/clients ───────────────────────────────────────────
void CommercialController::listClients(const HttpRequestPtr& req,
                                        std::function<void(const HttpResponsePtr&)>&& cb) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        auto clients = Client::findAll(claims->tenant_id);
        json arr = json::array();
        for (const auto& c : clients) arr.push_back(c.toJson());
        cb(jsonResp(k200OK, {{"clients", arr}, {"total", arr.size()}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// ── GET /api/v1/commercial/clients/{id} ──────────────────────────────────────
void CommercialController::getClient(const HttpRequestPtr& req,
                                      std::function<void(const HttpResponsePtr&)>&& cb,
                                      std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        auto client = Client::findById(id, claims->tenant_id);
        if (!client) { cb(jsonResp(k404NotFound, {{"error","Cliente no encontrado"}})); return; }

        json responseBody = client->toJson();

        // Adjuntar direcciones y contactos
        auto addr = PgConnection::getInstance().exec(
            "SELECT * FROM public.client_addresses WHERE client_id=$1 ORDER BY es_principal DESC", {id});
        auto contacts = PgConnection::getInstance().exec(
            "SELECT * FROM public.client_contacts WHERE client_id=$1 ORDER BY es_principal DESC", {id});

        json addrArr = json::array();
        for (int i = 0; i < addr.rows(); ++i) {
            addrArr.push_back({
                {"id",           addr.val(i,"id")},
                {"etiqueta",     addr.val(i,"etiqueta")},
                {"direccion",    addr.val(i,"direccion")},
                {"ciudad",       addr.val(i,"ciudad")},
                {"departamento", addr.val(i,"departamento")},
                {"pais",         addr.val(i,"pais")},
                {"es_principal", addr.val(i,"es_principal") == "t"}
            });
        }

        json contactArr = json::array();
        for (int i = 0; i < contacts.rows(); ++i) {
            contactArr.push_back({
                {"id",           contacts.val(i,"id")},
                {"nombre",       contacts.val(i,"nombre")},
                {"cargo",        contacts.val(i,"cargo")},
                {"email",        contacts.val(i,"email")},
                {"telefono1",    contacts.val(i,"telefono1")},
                {"telefono2",    contacts.val(i,"telefono2")},
                {"es_principal", contacts.val(i,"es_principal") == "t"}
            });
        }

        responseBody["addresses"] = addrArr;
        responseBody["contacts"]  = contactArr;
        cb(jsonResp(k200OK, {{"client", responseBody}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// ── POST /api/v1/commercial/visits ───────────────────────────────────────────
void CommercialController::createVisit(const HttpRequestPtr& req,
                                        std::function<void(const HttpResponsePtr&)>&& cb) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    json body;
    try { body = json::parse(req->getBody()); }
    catch (...) { cb(jsonResp(k400BadRequest, {{"error","JSON mal formado"}})); return; }

    auto req_str = [&](const char* key) -> bool {
        return body.contains(key) && body[key].is_string() && !body[key].get<std::string>().empty();
    };
    if (!req_str("client_id"))    { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: client_id"}}));   return; }
    if (!req_str("fecha_visita")) { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: fecha_visita"}})); return; }
    if (!req_str("tipo"))         { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: tipo"}}));         return; }
    if (!req_str("resumen"))      { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: resumen"}}));      return; }

    std::string tipo = body["tipo"].get<std::string>();
    if (tipo != "presencial" && tipo != "virtual" && tipo != "telefonica")
        { cb(jsonResp(k400BadRequest, {{"error","tipo debe ser: presencial, virtual o telefonica"}})); return; }

    try {
        std::string compromisos = body.contains("compromisos") && body["compromisos"].is_string()
            ? body["compromisos"].get<std::string>() : "";

        auto r = PgConnection::getInstance().exec(
            "INSERT INTO public.client_visits (client_id,commercial_id,tenant_id,fecha_visita,tipo,resumen,compromisos) "
            "VALUES ($1,$2,$3,$4::DATE,$5,$6,NULLIF($7,'')) RETURNING *",
            {body["client_id"].get<std::string>(), claims->user_id, claims->tenant_id,
             body["fecha_visita"].get<std::string>(), tipo,
             body["resumen"].get<std::string>(), compromisos}
        );

        if (!r.ok() || r.rows() == 0) throw std::runtime_error("insert failed");

        json visit = {
            {"id",           r.val(0,"id")},
            {"client_id",    r.val(0,"client_id")},
            {"commercial_id",r.val(0,"commercial_id")},
            {"fecha_visita", r.val(0,"fecha_visita")},
            {"tipo",         r.val(0,"tipo")},
            {"resumen",      r.val(0,"resumen")},
            {"compromisos",  r.val(0,"compromisos").empty() ? json(nullptr) : json(r.val(0,"compromisos"))},
            {"created_at",   r.val(0,"created_at")}
        };
        cb(jsonResp(k201Created, {{"visit", visit}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}
