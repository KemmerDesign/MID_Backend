#include "AuthController.h"
#include "../models/User.h"
#include "../utils/CryptoUtils.h"
#include <nlohmann/json.hpp>
#include <jwt-cpp/traits/nlohmann-json/defaults.h>
#include <fmt/core.h>
#include <cstdlib>
#include <chrono>

using json = nlohmann::json;

static constexpr const char* DEFAULT_JWT_SECRET = "monarca_dev_secret_CHANGE_IN_PROD";

static std::string jwtSecret() {
    const char* s = std::getenv("MONARCA_JWT_SECRET");
    return s ? s : DEFAULT_JWT_SECRET;
}

static HttpResponsePtr jsonResp(HttpStatusCode code, const json& body) {
    auto resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(code);
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    resp->setBody(body.dump());
    return resp;
}

void AuthController::login(const HttpRequestPtr& req,
                           std::function<void(const HttpResponsePtr&)>&& callback) {
    // 1. Parsear body
    json body;
    try {
        body = json::parse(req->getBody());
    } catch (...) {
        callback(jsonResp(k400BadRequest, {{"error", "JSON mal formado"}}));
        return;
    }

    // 2. Validar campos requeridos
    if (!body.contains("email") || !body["email"].is_string() || body["email"].get<std::string>().empty()) {
        callback(jsonResp(k400BadRequest, {{"error", "Campo requerido: email"}}));
        return;
    }
    if (!body.contains("password") || !body["password"].is_string() || body["password"].get<std::string>().empty()) {
        callback(jsonResp(k400BadRequest, {{"error", "Campo requerido: password"}}));
        return;
    }

    const std::string email    = body["email"].get<std::string>();
    const std::string password = body["password"].get<std::string>();

    // 3. Buscar usuario
    std::optional<User> maybeUser;
    try {
        maybeUser = User::findByEmail(email);
    } catch (const std::exception& e) {
        LOG_ERROR << "[AuthController] Error DB buscando usuario: " << e.what();
        callback(jsonResp(k500InternalServerError, {{"error", "Error interno del servidor"}}));
        return;
    }

    // 4. Verificar existencia y contraseña con el mismo mensaje para no filtrar info
    if (!maybeUser || !CryptoUtils::verifyPassword(password, maybeUser->password_hash)) {
        callback(jsonResp(k401Unauthorized, {{"error", "Credenciales inválidas"}}));
        return;
    }

    const User& user = *maybeUser;

    // 5. Verificar cuenta activa
    if (!user.active) {
        callback(jsonResp(k403Forbidden, {{"error", "Cuenta desactivada. Contacte al administrador."}}));
        return;
    }

    // 6. Generar JWT
    auto now = std::chrono::system_clock::now();
    std::string token;
    try {
        token = jwt::create()
            .set_issuer("monarca")
            .set_type("JWT")
            .set_issued_at(now)
            .set_expires_at(now + std::chrono::hours(24))
            .set_payload_claim("user_id",   jwt::claim(user.id))
            .set_payload_claim("tenant_id", jwt::claim(user.tenant_id))
            .set_payload_claim("role",      jwt::claim(user.role))
            .sign(jwt::algorithm::hs256{jwtSecret()});
    } catch (const std::exception& e) {
        LOG_ERROR << "[AuthController] Error generando JWT: " << e.what();
        callback(jsonResp(k500InternalServerError, {{"error", "Error interno del servidor"}}));
        return;
    }

    #ifdef MONARCA_DEV
        LOG_INFO << "[AuthController] Login exitoso: " << email << " role=" << user.role;
    #endif

    // 7. Responder
    json responseBody = {
        {"token", token},
        {"user", {
            {"id",        user.id},
            {"email",     user.email},
            {"full_name", user.full_name},
            {"role",      user.role},
            {"tenant_id", user.tenant_id}
        }}
    };

    callback(jsonResp(k200OK, responseBody));
}

void AuthController::register_(const HttpRequestPtr& req,
                                std::function<void(const HttpResponsePtr&)>&& callback) {
    // 1. Parsear body
    json body;
    try {
        body = json::parse(req->getBody());
    } catch (...) {
        callback(jsonResp(k400BadRequest, {{"error", "JSON mal formado"}}));
        return;
    }

    // 2. Validar campos requeridos
    auto requireString = [&](const char* field) -> std::optional<std::string> {
        if (!body.contains(field) || !body[field].is_string()) return std::nullopt;
        std::string v = body[field].get<std::string>();
        if (v.empty()) return std::nullopt;
        return v;
    };

    auto email     = requireString("email");
    auto password  = requireString("password");
    auto full_name = requireString("full_name");
    auto tenant_id = requireString("tenant_id");

    if (!email)     { callback(jsonResp(k400BadRequest, {{"error", "Campo requerido: email"}}));     return; }
    if (!password)  { callback(jsonResp(k400BadRequest, {{"error", "Campo requerido: password"}}));  return; }
    if (!full_name) { callback(jsonResp(k400BadRequest, {{"error", "Campo requerido: full_name"}})); return; }
    if (!tenant_id) { callback(jsonResp(k400BadRequest, {{"error", "Campo requerido: tenant_id"}})); return; }

    // Validación mínima de contraseña
    if (password->size() < 8) {
        callback(jsonResp(k400BadRequest, {{"error", "La contraseña debe tener al menos 8 caracteres"}}));
        return;
    }

    // Rol opcional (default: employee)
    std::string role = "employee";
    if (body.contains("role") && body["role"].is_string())
        role = body["role"].get<std::string>();

    if (role != "superadmin" && role != "admin" && role != "employee") {
        callback(jsonResp(k400BadRequest, {{"error", "Rol inválido. Valores permitidos: admin, employee"}}));
        return;
    }

    // 3. Verificar que el email no esté registrado
    try {
        if (User::findByEmail(*email).has_value()) {
            callback(jsonResp(k409Conflict, {{"error", "El email ya está registrado"}}));
            return;
        }
    } catch (const std::exception& e) {
        LOG_ERROR << "[AuthController] Error DB verificando email: " << e.what();
        callback(jsonResp(k500InternalServerError, {{"error", "Error interno del servidor"}}));
        return;
    }

    // 4. Hashear contraseña y crear usuario
    User newUser;
    try {
        std::string hash = CryptoUtils::hashPassword(*password);
        newUser = User::create(*tenant_id, *email, hash, *full_name, role);
    } catch (const std::exception& e) {
        LOG_ERROR << "[AuthController] Error creando usuario: " << e.what();
        callback(jsonResp(k500InternalServerError, {{"error", "Error interno del servidor"}}));
        return;
    }

    #ifdef MONARCA_DEV
        LOG_INFO << "[AuthController] Usuario registrado: " << *email << " role=" << role;
    #endif

    // 5. Responder con el usuario creado (sin token — debe hacer login después)
    json responseBody = {
        {"message", "Usuario registrado exitosamente"},
        {"user", {
            {"id",        newUser.id},
            {"email",     newUser.email},
            {"full_name", newUser.full_name},
            {"role",      newUser.role},
            {"tenant_id", newUser.tenant_id}
        }}
    };

    callback(jsonResp(k201Created, responseBody));
}
