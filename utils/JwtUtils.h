#pragma once
#include <string>
#include <optional>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

using namespace drogon;

struct AuthClaims {
    std::string user_id;
    std::string tenant_id;
    std::string role;
};

namespace JwtUtils {
    std::optional<AuthClaims> extractClaims(const HttpRequestPtr& req);
    HttpResponsePtr            unauthorized();
}
