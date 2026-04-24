#include "Client.h"
#include <libpq-fe.h>
#include <stdexcept>

static Client rowToClient(const PgResult& r, int row) {
    Client c;
    c.id                  = r.val(row, "id");
    c.tenant_id           = r.val(row, "tenant_id");
    c.nombre              = r.val(row, "nombre");
    c.nombre_comercial    = r.val(row, "nombre_comercial");
    c.nit                 = r.val(row, "nit");
    c.direccion_principal = r.val(row, "direccion_principal");
    c.ciudad_principal    = r.val(row, "ciudad_principal");
    c.correo_facturacion  = r.val(row, "correo_facturacion");
    c.sitio_web           = r.val(row, "sitio_web");
    c.sector              = r.val(row, "sector");
    c.activo              = r.val(row, "activo") == "t";
    c.notas               = r.val(row, "notas");
    c.created_at          = r.val(row, "created_at");
    c.updated_at          = r.val(row, "updated_at");
    return c;
}

json Client::toJson() const {
    auto nullable = [](const std::string& s) -> json {
        return s.empty() ? json(nullptr) : json(s);
    };
    return {
        {"id",                  id},
        {"tenant_id",           tenant_id},
        {"nombre",              nombre},
        {"nombre_comercial",    nullable(nombre_comercial)},
        {"nit",                 nit},
        {"direccion_principal", direccion_principal},
        {"ciudad_principal",    ciudad_principal},
        {"correo_facturacion",  nullable(correo_facturacion)},
        {"sitio_web",           nullable(sitio_web)},
        {"sector",              nullable(sector)},
        {"activo",              activo},
        {"notas",               nullable(notas)},
        {"created_at",          created_at},
        {"updated_at",          updated_at}
    };
}

Client Client::create(const std::string& tenant_id, const json& data) {
    auto str = [&](const char* key, const char* def = "") -> std::string {
        if (data.contains(key) && data[key].is_string()) return data[key].get<std::string>();
        return def;
    };

    auto result = PgConnection::getInstance().exec(
        "INSERT INTO public.clients "
        "(tenant_id,nombre,nombre_comercial,nit,direccion_principal,ciudad_principal,"
        " correo_facturacion,sitio_web,sector,notas) "
        "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10) RETURNING *",
        {tenant_id,
         str("nombre"),
         str("nombre_comercial"),
         str("nit"),
         str("direccion_principal"),
         str("ciudad_principal", "Bogotá"),
         str("correo_facturacion"),
         str("sitio_web"),
         str("sector"),
         str("notas")}
    );

    if (!result.ok() || result.rows() == 0)
        throw std::runtime_error(std::string("Client::create: ") +
                                 (result.res ? PQresultErrorMessage(result.res) : "sin resultado"));
    return rowToClient(result, 0);
}

std::optional<Client> Client::findById(const std::string& id, const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "SELECT * FROM public.clients WHERE id=$1 AND tenant_id=$2 LIMIT 1", {id, tenant_id});
    if (!r.ok() || r.rows() == 0) return std::nullopt;
    return rowToClient(r, 0);
}

std::vector<Client> Client::findAll(const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "SELECT * FROM public.clients WHERE tenant_id=$1 ORDER BY nombre ASC", {tenant_id});
    std::vector<Client> out;
    if (!r.ok()) return out;
    for (int i = 0; i < r.rows(); ++i) out.push_back(rowToClient(r, i));
    return out;
}
