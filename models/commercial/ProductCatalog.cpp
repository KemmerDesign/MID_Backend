#include "ProductCatalog.h"
#include <stdexcept>

static ProductCatalog rowToCatalog(const PgResult& r, int row) {
    ProductCatalog c;
    c.id          = r.val(row, "id");
    c.tenant_id   = r.val(row, "tenant_id");
    c.nombre      = r.val(row, "nombre");
    c.categoria   = r.val(row, "categoria");
    c.descripcion = r.val(row, "descripcion");
    c.activo      = r.val(row, "activo") == "t";
    c.created_at  = r.val(row, "created_at");
    c.updated_at  = r.val(row, "updated_at");
    return c;
}

json ProductCatalog::toJson() const {
    return {
        {"id",          id},
        {"tenant_id",   tenant_id},
        {"nombre",      nombre},
        {"categoria",   categoria},
        {"descripcion", descripcion.empty() ? json(nullptr) : json(descripcion)},
        {"activo",      activo},
        {"created_at",  created_at},
        {"updated_at",  updated_at}
    };
}

ProductCatalog ProductCatalog::create(const std::string& tenant_id, const json& data) {
    auto str = [&](const char* key, const char* def = "") -> std::string {
        return (data.contains(key) && data[key].is_string()) ? data[key].get<std::string>() : def;
    };

    auto r = PgConnection::getInstance().exec(
        "INSERT INTO public.product_catalog (tenant_id, nombre, categoria, descripcion) "
        "VALUES ($1, $2, $3, NULLIF($4,'')) RETURNING *",
        {tenant_id, str("nombre"), str("categoria"), str("descripcion")}
    );

    if (!r.ok() || r.rows() == 0)
        throw PgException(std::string("ProductCatalog::create: ") + r.errMsg(), r.pgCode());
    return rowToCatalog(r, 0);
}

std::vector<ProductCatalog> ProductCatalog::findAll(const std::string& tenant_id, bool only_active) {
    PgResult r = only_active
        ? PgConnection::getInstance().exec(
            "SELECT * FROM public.product_catalog WHERE tenant_id=$1 AND activo=TRUE "
            "ORDER BY categoria, nombre", {tenant_id})
        : PgConnection::getInstance().exec(
            "SELECT * FROM public.product_catalog WHERE tenant_id=$1 "
            "ORDER BY categoria, nombre", {tenant_id});

    std::vector<ProductCatalog> out;
    if (!r.ok()) return out;
    for (int i = 0; i < r.rows(); ++i) out.push_back(rowToCatalog(r, i));
    return out;
}

std::optional<ProductCatalog> ProductCatalog::findById(const std::string& id,
                                                        const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "SELECT * FROM public.product_catalog WHERE id=$1 AND tenant_id=$2 LIMIT 1",
        {id, tenant_id});
    if (!r.ok() || r.rows() == 0) return std::nullopt;
    return rowToCatalog(r, 0);
}

std::optional<ProductCatalog> ProductCatalog::update(const std::string& id,
                                                       const std::string& tenant_id,
                                                       const json& data) {
    // Solo actualiza los campos presentes en el body (PATCH parcial)
    std::string sql =
        "UPDATE public.product_catalog SET "
        "  nombre      = COALESCE(NULLIF($3,''), nombre), "
        "  categoria   = COALESCE(NULLIF($4,''), categoria), "
        "  descripcion = CASE WHEN $5 = '__unset__' THEN descripcion "
        "                     ELSE NULLIF($5,'') END, "
        "  activo      = CASE WHEN $6 = '' THEN activo ELSE ($6='true')::BOOLEAN END "
        "WHERE id=$1 AND tenant_id=$2 RETURNING *";

    auto str = [&](const char* key, const char* def = "") -> std::string {
        return (data.contains(key) && data[key].is_string()) ? data[key].get<std::string>() : def;
    };

    std::string activo_str;
    if (data.contains("activo") && data["activo"].is_boolean())
        activo_str = data["activo"].get<bool>() ? "true" : "false";

    std::string desc = "__unset__";
    if (data.contains("descripcion")) {
        if (data["descripcion"].is_string()) desc = data["descripcion"].get<std::string>();
        else if (data["descripcion"].is_null()) desc = "";
    }

    auto r = PgConnection::getInstance().exec(sql,
        {id, tenant_id, str("nombre"), str("categoria"), desc, activo_str});
    if (!r.ok() || r.rows() == 0) return std::nullopt;
    return rowToCatalog(r, 0);
}
