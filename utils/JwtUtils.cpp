#include "JwtUtils.h"
#include <jwt-cpp/traits/nlohmann-json/defaults.h>
#include <nlohmann/json.hpp>
#include <cstdlib>

static constexpr const char* DEFAULT_JWT_SECRET = "monarca_dev_secret_CHANGE_IN_PROD";

static std::string jwtSecret() {
    const char* s = std::getenv("MONARCA_JWT_SECRET");
    return s ? s : DEFAULT_JWT_SECRET;
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
