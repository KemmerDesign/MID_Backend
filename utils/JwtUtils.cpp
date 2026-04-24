#include "JwtUtils.h"
#include <jwt-cpp/traits/nlohmann-json/defaults.h>
#include <nlohmann/json.hpp>
#include <fmt/core.h>
#include <cstdlib>

// El secreto se resuelve UNA SOLA VEZ al primer uso (static local).
// En dev: usa la env var o el fallback de desarrollo.
// En producción: abort si la env var no está definida — nunca arrancar sin secreto real.
static const std::string& jwtSecret() {
    static const std::string secret = []() -> std::string {
        const char* s = std::getenv("MONARCA_JWT_SECRET");
#ifdef MONARCA_DEV
        return s ? s : "monarca_dev_secret_CHANGE_IN_PROD";
#else
        if (!s) {
            fmt::print(stderr,
                "❌ [PROD] Variable de entorno 'MONARCA_JWT_SECRET' no definida. "
                "El servidor no arranca sin un secreto JWT seguro.\n");
            std::abort();
        }
        return s;
#endif
    }();
    return secret;
}

namespace JwtUtils {

std::optional<AuthClaims> extractClaims(const HttpRequestPtr& req) {
    std::string auth = req->getHeader("Authorization");
    if (auth.size() < 8 || auth.substr(0, 7) != "Bearer ")
        return std::nullopt;

    try {
        auto decoded = jwt::decode(auth.substr(7));
        jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{jwtSecret()})
            .with_issuer("monarca")
            .verify(decoded);

        return AuthClaims{
            decoded.get_payload_claim("user_id").as_string(),
            decoded.get_payload_claim("tenant_id").as_string(),
            decoded.get_payload_claim("role").as_string()
        };
    } catch (...) {
        return std::nullopt;
    }
}

HttpResponsePtr unauthorized() {
    auto resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k401Unauthorized);
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    resp->setBody(R"({"error":"Token inválido o no proporcionado"})");
    return resp;
}

} // namespace JwtUtils
