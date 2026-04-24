#include "User.h"
#include <libpq-fe.h>
#include <stdexcept>

User User::create(const std::string& tenant_id,
                  const std::string& email,
                  const std::string& password_hash,
                  const std::string& full_name,
                  const std::string& role) {
    auto result = PgConnection::getInstance().exec(
        "INSERT INTO public.users (tenant_id, email, password_hash, full_name, role) "
        "VALUES ($1, $2, $3, $4, $5) "
        "RETURNING id, tenant_id, email, full_name, role, active",
        {tenant_id, email, password_hash, full_name, role}
    );

    if (!result.ok() || result.rows() == 0) {
        // PQresultErrorMessage nos da el detalle (ej. violación de UNIQUE)
        throw PgException(std::string("User::create falló: ") + result.errMsg(), result.pgCode());
    }

    User u;
    u.id            = result.val(0, "id");
    u.tenant_id     = result.val(0, "tenant_id");
    u.email         = result.val(0, "email");
    u.password_hash = password_hash;
    u.full_name     = result.val(0, "full_name");
    u.role          = result.val(0, "role");
    u.active        = result.val(0, "active") == "t";
    return u;
}

std::optional<User> User::findByEmail(const std::string& email) {
    auto result = PgConnection::getInstance().exec(
        "SELECT id, tenant_id, email, password_hash, full_name, role, active "
        "FROM public.users WHERE email = $1 LIMIT 1",
        {email}
    );

    if (!result.ok() || result.rows() == 0)
        return std::nullopt;

    User u;
    u.id            = result.val(0, "id");
    u.tenant_id     = result.val(0, "tenant_id");
    u.email         = result.val(0, "email");
    u.password_hash = result.val(0, "password_hash");
    u.full_name     = result.val(0, "full_name");
    u.role          = result.val(0, "role");
    u.active        = result.val(0, "active") == "t";  // PostgreSQL bool → "t"/"f"

    return u;
}
