#include "EmployeeDocument.h"
#include <stdexcept>

static EmployeeDocument rowToDocument(const PgResult& r, int i) {
    EmployeeDocument d;
    d.id               = r.val(i, "id");
    d.employee_id      = r.val(i, "employee_id");
    d.tenant_id        = r.val(i, "tenant_id");
    d.tipo             = r.val(i, "tipo");
    d.nombre           = r.val(i, "nombre");
    d.url_referencia   = r.val(i, "url_referencia");
    d.fecha_entrega    = r.val(i, "fecha_entrega");
    d.fecha_vencimiento = r.val(i, "fecha_vencimiento");
    d.obligatorio      = r.val(i, "obligatorio") == "t";
    d.estado           = r.val(i, "estado");
    d.notas            = r.val(i, "notas");
    d.created_at       = r.val(i, "created_at");
    d.updated_at       = r.val(i, "updated_at");
    return d;
}

json EmployeeDocument::toJson() const {
    auto nul = [](const std::string& s) -> json { return s.empty() ? json(nullptr) : json(s); };
    return {
        {"id",               id},
        {"employee_id",      employee_id},
        {"tenant_id",        tenant_id},
        {"tipo",             tipo},
        {"nombre",           nombre},
        {"url_referencia",   nul(url_referencia)},
        {"fecha_entrega",    nul(fecha_entrega)},
        {"fecha_vencimiento",nul(fecha_vencimiento)},
        {"obligatorio",      obligatorio},
        {"estado",           estado},
        {"notas",            nul(notas)},
        {"created_at",       created_at},
        {"updated_at",       updated_at}
    };
}

EmployeeDocument EmployeeDocument::create(const std::string& employee_id,
                                           const std::string& tenant_id,
                                           const json& data) {
    auto str = [&](const char* k, const char* def = "") -> std::string {
        return (data.contains(k) && data[k].is_string()) ? data[k].get<std::string>() : def;
    };
    bool obligatorio = true;
    if (data.contains("obligatorio") && data["obligatorio"].is_boolean())
        obligatorio = data["obligatorio"].get<bool>();

    auto r = PgConnection::getInstance().exec(
        "INSERT INTO public.employee_documents "
        "(employee_id, tenant_id, tipo, nombre, url_referencia, "
        " fecha_entrega, fecha_vencimiento, obligatorio, notas) "
        "VALUES ($1,$2,$3,$4,NULLIF($5,''),"
        "        NULLIF($6,'')::DATE,NULLIF($7,'')::DATE,$8,NULLIF($9,'')) "
        "RETURNING *",
        {employee_id, tenant_id,
         str("tipo"), str("nombre"), str("url_referencia"),
         str("fecha_entrega"), str("fecha_vencimiento"),
         obligatorio ? "true" : "false",
         str("notas")}
    );
    if (!r.ok() || r.rows() == 0)
        throw PgException(std::string("EmployeeDocument::create: ") + r.errMsg(), r.pgCode());
    return rowToDocument(r, 0);
}

std::vector<EmployeeDocument> EmployeeDocument::findByEmployee(const std::string& employee_id,
                                                                 const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "SELECT * FROM public.employee_documents "
        "WHERE employee_id=$1 AND tenant_id=$2 ORDER BY tipo, nombre",
        {employee_id, tenant_id});
    std::vector<EmployeeDocument> out;
    if (!r.ok()) return out;
    for (int i = 0; i < r.rows(); ++i) out.push_back(rowToDocument(r, i));
    return out;
}

std::optional<EmployeeDocument> EmployeeDocument::updateStatus(const std::string& id,
                                                                 const std::string& employee_id,
                                                                 const std::string& tenant_id,
                                                                 const json& data) {
    auto str = [&](const char* k, const char* def = "") -> std::string {
        return (data.contains(k) && data[k].is_string()) ? data[k].get<std::string>() : def;
    };
    auto r = PgConnection::getInstance().exec(
        "UPDATE public.employee_documents "
        "SET estado=COALESCE(NULLIF($3,''), estado), "
        "    url_referencia=COALESCE(NULLIF($4,''), url_referencia), "
        "    fecha_entrega=COALESCE(NULLIF($5,'')::DATE, fecha_entrega), "
        "    fecha_vencimiento=COALESCE(NULLIF($6,'')::DATE, fecha_vencimiento), "
        "    notas=COALESCE(NULLIF($7,''), notas) "
        "WHERE id=$1 AND employee_id=$2 AND tenant_id=$8 RETURNING *",
        {id, employee_id,
         str("estado"), str("url_referencia"),
         str("fecha_entrega"), str("fecha_vencimiento"), str("notas"),
         tenant_id}
    );
    if (!r.ok() || r.rows() == 0) return std::nullopt;
    return rowToDocument(r, 0);
}

bool EmployeeDocument::remove(const std::string& id,
                               const std::string& employee_id,
                               const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "DELETE FROM public.employee_documents "
        "WHERE id=$1 AND employee_id=$2 AND tenant_id=$3 RETURNING id",
        {id, employee_id, tenant_id});
    return r.cmdOk() || (r.ok() && r.rows() > 0);
}
