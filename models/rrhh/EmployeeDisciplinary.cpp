#include "EmployeeDisciplinary.h"
#include <stdexcept>

static EmployeeDisciplinary rowToDisciplinary(const PgResult& r, int i) {
    EmployeeDisciplinary d;
    d.id                   = r.val(i, "id");
    d.employee_id          = r.val(i, "employee_id");
    d.tenant_id            = r.val(i, "tenant_id");
    d.tipo                 = r.val(i, "tipo");
    d.fecha                = r.val(i, "fecha");
    d.motivo               = r.val(i, "motivo");
    d.descripcion          = r.val(i, "descripcion");
    d.estado               = r.val(i, "estado");
    d.firmado_por_empleado = r.val(i, "firmado_por_empleado") == "t";
    d.fecha_firma          = r.val(i, "fecha_firma");
    d.created_by           = r.val(i, "created_by");
    d.created_at           = r.val(i, "created_at");
    d.updated_at           = r.val(i, "updated_at");
    return d;
}

json EmployeeDisciplinary::toJson() const {
    auto nul = [](const std::string& s) -> json { return s.empty() ? json(nullptr) : json(s); };
    return {
        {"id",                   id},
        {"employee_id",          employee_id},
        {"tenant_id",            tenant_id},
        {"tipo",                 tipo},
        {"fecha",                fecha},
        {"motivo",               motivo},
        {"descripcion",          nul(descripcion)},
        {"estado",               estado},
        {"firmado_por_empleado", firmado_por_empleado},
        {"fecha_firma",          nul(fecha_firma)},
        {"created_by",           nul(created_by)},
        {"created_at",           created_at},
        {"updated_at",           updated_at}
    };
}

EmployeeDisciplinary EmployeeDisciplinary::create(const std::string& employee_id,
                                                    const std::string& tenant_id,
                                                    const std::string& created_by,
                                                    const json& data) {
    auto str = [&](const char* k, const char* def = "") -> std::string {
        return (data.contains(k) && data[k].is_string()) ? data[k].get<std::string>() : def;
    };

    auto r = PgConnection::getInstance().exec(
        "INSERT INTO public.employee_disciplinary "
        "(employee_id, tenant_id, tipo, fecha, motivo, descripcion, created_by) "
        "VALUES ($1,$2,$3,$4::DATE,$5,NULLIF($6,''),NULLIF($7,'')::UUID) "
        "RETURNING *",
        {employee_id, tenant_id,
         str("tipo"), str("fecha"), str("motivo"),
         str("descripcion"), created_by}
    );
    if (!r.ok() || r.rows() == 0)
        throw std::runtime_error(std::string("EmployeeDisciplinary::create: ") + r.errMsg());
    return rowToDisciplinary(r, 0);
}

std::vector<EmployeeDisciplinary> EmployeeDisciplinary::findByEmployee(const std::string& employee_id,
                                                                         const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "SELECT * FROM public.employee_disciplinary "
        "WHERE employee_id=$1 AND tenant_id=$2 ORDER BY fecha DESC",
        {employee_id, tenant_id});
    std::vector<EmployeeDisciplinary> out;
    if (!r.ok()) return out;
    for (int i = 0; i < r.rows(); ++i) out.push_back(rowToDisciplinary(r, i));
    return out;
}

std::optional<EmployeeDisciplinary> EmployeeDisciplinary::update(const std::string& id,
                                                                   const std::string& employee_id,
                                                                   const std::string& tenant_id,
                                                                   const json& data) {
    auto str = [&](const char* k) -> std::string {
        return (data.contains(k) && data[k].is_string()) ? data[k].get<std::string>() : "";
    };
    std::string firmado_s;
    if (data.contains("firmado_por_empleado") && data["firmado_por_empleado"].is_boolean())
        firmado_s = data["firmado_por_empleado"].get<bool>() ? "true" : "false";

    auto r = PgConnection::getInstance().exec(
        "UPDATE public.employee_disciplinary SET "
        "  estado               = CASE WHEN $4='' THEN estado       ELSE $4 END, "
        "  firmado_por_empleado = CASE WHEN $5='' THEN firmado_por_empleado ELSE ($5='true') END, "
        "  fecha_firma          = CASE WHEN $6='' THEN fecha_firma   ELSE $6::DATE END, "
        "  descripcion          = CASE WHEN $7='' THEN descripcion   ELSE NULLIF($7,'') END "
        "WHERE id=$1 AND employee_id=$2 AND tenant_id=$3 RETURNING *",
        {id, employee_id, tenant_id, str("estado"), firmado_s, str("fecha_firma"), str("descripcion")}
    );
    if (!r.ok() || r.rows() == 0) return std::nullopt;
    return rowToDisciplinary(r, 0);
}
