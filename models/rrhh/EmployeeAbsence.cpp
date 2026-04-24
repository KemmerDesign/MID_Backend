#include "EmployeeAbsence.h"
#include <stdexcept>

static EmployeeAbsence rowToAbsence(const PgResult& r, int i) {
    EmployeeAbsence a;
    a.id                = r.val(i, "id");
    a.employee_id       = r.val(i, "employee_id");
    a.tenant_id         = r.val(i, "tenant_id");
    a.tipo              = r.val(i, "tipo");
    a.fecha_inicio      = r.val(i, "fecha_inicio");
    a.fecha_fin         = r.val(i, "fecha_fin");
    a.dias_calendario   = r.val(i, "dias_calendario");
    a.remunerado        = r.val(i, "remunerado") == "t";
    a.descripcion       = r.val(i, "descripcion");
    a.documento_soporte = r.val(i, "documento_soporte");
    a.estado            = r.val(i, "estado");
    a.aprobado_por      = r.val(i, "aprobado_por");
    a.created_by        = r.val(i, "created_by");
    a.descuenta_domingo = r.val(i, "descuenta_domingo") == "t";
    a.domingo_afectado  = r.val(i, "domingo_afectado");
    a.created_at        = r.val(i, "created_at");
    a.updated_at        = r.val(i, "updated_at");
    return a;
}

json EmployeeAbsence::toJson() const {
    auto nul = [](const std::string& s) -> json { return s.empty() ? json(nullptr) : json(s); };
    auto intVal = [](const std::string& s) -> json {
        if (s.empty()) return json(nullptr);
        try { return json(std::stoi(s)); } catch (...) { return json(nullptr); }
    };
    return {
        {"id",                id},
        {"employee_id",       employee_id},
        {"tenant_id",         tenant_id},
        {"tipo",              tipo},
        {"fecha_inicio",      fecha_inicio},
        {"fecha_fin",         fecha_fin},
        {"dias_calendario",   intVal(dias_calendario)},
        {"remunerado",        remunerado},
        {"descripcion",       nul(descripcion)},
        {"documento_soporte", nul(documento_soporte)},
        {"estado",            estado},
        {"aprobado_por",      nul(aprobado_por)},
        {"created_by",        nul(created_by)},
        {"descuenta_domingo", descuenta_domingo},
        {"domingo_afectado",  nul(domingo_afectado)},
        {"created_at",        created_at},
        {"updated_at",        updated_at}
    };
}

EmployeeAbsence EmployeeAbsence::create(const std::string& employee_id,
                                         const std::string& tenant_id,
                                         const std::string& created_by,
                                         const json& data) {
    auto str = [&](const char* k, const char* def = "") -> std::string {
        return (data.contains(k) && data[k].is_string()) ? data[k].get<std::string>() : def;
    };
    bool remunerado = true;
    if (data.contains("remunerado") && data["remunerado"].is_boolean())
        remunerado = data["remunerado"].get<bool>();

    bool descuenta = false;
    if (data.contains("descuenta_domingo") && data["descuenta_domingo"].is_boolean())
        descuenta = data["descuenta_domingo"].get<bool>();

    auto r = PgConnection::getInstance().exec(
        "INSERT INTO public.employee_absences "
        "(employee_id, tenant_id, tipo, fecha_inicio, fecha_fin, remunerado, "
        " descripcion, documento_soporte, descuenta_domingo, domingo_afectado, created_by) "
        "VALUES ($1,$2,$3,$4::DATE,$5::DATE,$6,"
        "        NULLIF($7,''),NULLIF($8,''),$9,NULLIF($10,'')::DATE,NULLIF($11,'')::UUID) "
        "RETURNING *",
        {employee_id, tenant_id, str("tipo"),
         str("fecha_inicio"), str("fecha_fin"),
         remunerado ? "true" : "false",
         str("descripcion"), str("documento_soporte"),
         descuenta ? "true" : "false",
         str("domingo_afectado"),
         created_by}
    );
    if (!r.ok() || r.rows() == 0)
        throw PgException(std::string("EmployeeAbsence::create: ") + r.errMsg(), r.pgCode());
    return rowToAbsence(r, 0);
}

std::vector<EmployeeAbsence> EmployeeAbsence::findByEmployee(const std::string& employee_id,
                                                               const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "SELECT * FROM public.employee_absences "
        "WHERE employee_id=$1 AND tenant_id=$2 ORDER BY fecha_inicio DESC",
        {employee_id, tenant_id});
    std::vector<EmployeeAbsence> out;
    if (!r.ok()) return out;
    for (int i = 0; i < r.rows(); ++i) out.push_back(rowToAbsence(r, i));
    return out;
}

std::optional<EmployeeAbsence> EmployeeAbsence::updateStatus(const std::string& id,
                                                               const std::string& employee_id,
                                                               const std::string& tenant_id,
                                                               const std::string& estado,
                                                               const std::string& aprobado_por) {
    auto r = PgConnection::getInstance().exec(
        "UPDATE public.employee_absences "
        "SET estado=$3, aprobado_por=NULLIF($4,'')::UUID "
        "WHERE id=$1 AND employee_id=$2 AND tenant_id=$5 RETURNING *",
        {id, employee_id, estado, aprobado_por, tenant_id}
    );
    if (!r.ok() || r.rows() == 0) return std::nullopt;
    return rowToAbsence(r, 0);
}
