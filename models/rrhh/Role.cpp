#include "Role.h"
#include <stdexcept>

static Role rowToRole(const PgResult& r, int i) {
    Role ro;
    ro.id               = r.val(i, "id");
    ro.tenant_id        = r.val(i, "tenant_id");
    ro.nombre           = r.val(i, "nombre");
    ro.descripcion      = r.val(i, "descripcion");
    ro.departamento     = r.val(i, "departamento");
    const std::string nj = r.val(i, "nivel_jerarquico");
    ro.nivel_jerarquico = nj.empty() ? 1 : std::stoi(nj);
    ro.activo           = r.val(i, "activo") == "t";
    ro.created_at       = r.val(i, "created_at");
    ro.updated_at       = r.val(i, "updated_at");
    return ro;
}

json Role::toJson() const {
    return {
        {"id",               id},
        {"tenant_id",        tenant_id},
        {"nombre",           nombre},
        {"descripcion",      descripcion.empty() ? json(nullptr) : json(descripcion)},
        {"departamento",     departamento},
        {"nivel_jerarquico", nivel_jerarquico},
        {"activo",           activo},
        {"created_at",       created_at},
        {"updated_at",       updated_at}
    };
}

Role Role::create(const std::string& tenant_id, const json& data) {
    auto str = [&](const char* k, const char* def = "") -> std::string {
        return (data.contains(k) && data[k].is_string()) ? data[k].get<std::string>() : def;
    };
    int nivel = 1;
    if (data.contains("nivel_jerarquico") && data["nivel_jerarquico"].is_number_integer())
        nivel = data["nivel_jerarquico"].get<int>();

    auto r = PgConnection::getInstance().exec(
        "INSERT INTO public.roles (tenant_id, nombre, descripcion, departamento, nivel_jerarquico) "
        "VALUES ($1, $2, NULLIF($3,''), $4, $5) RETURNING *",
        {tenant_id, str("nombre"), str("descripcion"), str("departamento"),
         std::to_string(nivel)}
    );
    if (!r.ok() || r.rows() == 0)
        throw std::runtime_error(std::string("Role::create: ") + r.errMsg());
    return rowToRole(r, 0);
}

std::vector<Role> Role::findAll(const std::string& tenant_id, const std::string& departamento) {
    PgResult r = departamento.empty()
        ? PgConnection::getInstance().exec(
            "SELECT * FROM public.roles WHERE tenant_id=$1 ORDER BY departamento, nivel_jerarquico, nombre",
            {tenant_id})
        : PgConnection::getInstance().exec(
            "SELECT * FROM public.roles WHERE tenant_id=$1 AND departamento=$2 "
            "ORDER BY nivel_jerarquico, nombre",
            {tenant_id, departamento});

    std::vector<Role> out;
    if (!r.ok()) return out;
    for (int i = 0; i < r.rows(); ++i) out.push_back(rowToRole(r, i));
    return out;
}

std::optional<Role> Role::findById(const std::string& id, const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "SELECT * FROM public.roles WHERE id=$1 AND tenant_id=$2 LIMIT 1", {id, tenant_id});
    if (!r.ok() || r.rows() == 0) return std::nullopt;
    return rowToRole(r, 0);
}

std::optional<Role> Role::update(const std::string& id, const std::string& tenant_id,
                                  const json& data) {
    auto str = [&](const char* k) -> std::string {
        return (data.contains(k) && data[k].is_string()) ? data[k].get<std::string>() : "";
    };
    std::string nivel_s;
    if (data.contains("nivel_jerarquico") && data["nivel_jerarquico"].is_number_integer())
        nivel_s = std::to_string(data["nivel_jerarquico"].get<int>());

    std::string activo_s;
    if (data.contains("activo") && data["activo"].is_boolean())
        activo_s = data["activo"].get<bool>() ? "true" : "false";

    auto r = PgConnection::getInstance().exec(
        "UPDATE public.roles SET "
        "  nombre           = COALESCE(NULLIF($3,''), nombre), "
        "  descripcion      = CASE WHEN $4='' THEN descripcion ELSE NULLIF($4,'') END, "
        "  departamento     = COALESCE(NULLIF($5,''), departamento), "
        "  nivel_jerarquico = CASE WHEN $6='' THEN nivel_jerarquico ELSE $6::INTEGER END, "
        "  activo           = CASE WHEN $7='' THEN activo ELSE ($7='true')::BOOLEAN END "
        "WHERE id=$1 AND tenant_id=$2 RETURNING *",
        {id, tenant_id, str("nombre"), str("descripcion"),
         str("departamento"), nivel_s, activo_s}
    );
    if (!r.ok() || r.rows() == 0) return std::nullopt;
    return rowToRole(r, 0);
}
