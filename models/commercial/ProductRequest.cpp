#include "ProductRequest.h"
#include <libpq-fe.h>
#include <stdexcept>

static ProductRequest rowToPR(const PgResult& r, int row) {
    ProductRequest p;
    p.id                    = r.val(row, "id");
    p.project_id            = r.val(row, "project_id");
    p.tenant_id             = r.val(row, "tenant_id");
    p.commercial_id         = r.val(row, "commercial_id");
    p.nombre_producto       = r.val(row, "nombre_producto");
    p.descripcion_necesidad = r.val(row, "descripcion_necesidad");
    std::string cant        = r.val(row, "cantidad_estimada");
    p.cantidad_estimada     = cant.empty() ? 0 : std::stoi(cant);
    p.presupuesto_estimado  = r.val(row, "presupuesto_estimado");
    p.unidad_medida         = r.val(row, "unidad_medida");
    p.estado                = r.val(row, "estado");
    p.notas_adicionales     = r.val(row, "notas_adicionales");
    p.created_at            = r.val(row, "created_at");
    p.updated_at            = r.val(row, "updated_at");
    return p;
}

json ProductRequest::toJson() const {
    auto nullable = [](const std::string& s) -> json {
        return s.empty() ? json(nullptr) : json(s);
    };
    json j = {
        {"id",                    id},
        {"project_id",            project_id},
        {"tenant_id",             tenant_id},
        {"commercial_id",         commercial_id},
        {"nombre_producto",       nombre_producto},
        {"descripcion_necesidad", descripcion_necesidad},
        {"cantidad_estimada",     cantidad_estimada},
        {"unidad_medida",         unidad_medida},
        {"estado",                estado},
        {"notas_adicionales",     nullable(notas_adicionales)},
        {"created_at",            created_at},
        {"updated_at",            updated_at}
    };
    j["presupuesto_estimado"] = presupuesto_estimado.empty()
        ? json(nullptr)
        : json(std::stod(presupuesto_estimado));
    return j;
}

ProductRequest ProductRequest::create(const std::string& project_id,
                                       const std::string& tenant_id,
                                       const std::string& commercial_id,
                                       const json& data) {
    auto str = [&](const char* key, const char* def = "") -> std::string {
        if (data.contains(key) && data[key].is_string()) return data[key].get<std::string>();
        return def;
    };
    std::string cantidad = data.contains("cantidad_estimada") && data["cantidad_estimada"].is_number()
        ? std::to_string(data["cantidad_estimada"].get<int>()) : "1";
    std::string presupuesto = "";
    if (data.contains("presupuesto_estimado") && data["presupuesto_estimado"].is_number())
        presupuesto = std::to_string(data["presupuesto_estimado"].get<double>());

    auto r = PgConnection::getInstance().exec(
        "INSERT INTO public.product_requests "
        "(project_id,tenant_id,commercial_id,nombre_producto,descripcion_necesidad,"
        " cantidad_estimada,presupuesto_estimado,unidad_medida,notas_adicionales) "
        "VALUES ($1,$2,$3,$4,$5,$6::INTEGER,NULLIF($7,'')::NUMERIC,$8,NULLIF($9,'')) "
        "RETURNING *",
        {project_id, tenant_id, commercial_id,
         str("nombre_producto"), str("descripcion_necesidad"),
         cantidad, presupuesto,
         str("unidad_medida", "unidad"),
         str("notas_adicionales")}
    );

    if (!r.ok() || r.rows() == 0)
        throw std::runtime_error(std::string("ProductRequest::create: ") +
                                 (r.res ? PQresultErrorMessage(r.res) : "sin resultado"));
    return rowToPR(r, 0);
}

std::vector<ProductRequest> ProductRequest::findByProject(const std::string& project_id) {
    auto r = PgConnection::getInstance().exec(
        "SELECT * FROM public.product_requests WHERE project_id=$1 ORDER BY created_at ASC",
        {project_id});
    std::vector<ProductRequest> out;
    if (!r.ok()) return out;
    for (int i = 0; i < r.rows(); ++i) out.push_back(rowToPR(r, i));
    return out;
}
