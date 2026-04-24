#include "EmployeeDependent.h"
#include <stdexcept>

static EmployeeDependent rowToDependent(const PgResult& r, int i) {
    EmployeeDependent d;
    d.id                = r.val(i, "id");
    d.employee_id       = r.val(i, "employee_id");
    d.tenant_id         = r.val(i, "tenant_id");
    d.nombres           = r.val(i, "nombres");
    d.apellidos         = r.val(i, "apellidos");
    d.fecha_nacimiento  = r.val(i, "fecha_nacimiento");
    d.parentesco        = r.val(i, "parentesco");
    d.vive_con_empleado = r.val(i, "vive_con_empleado") == "t";
    d.activo            = r.val(i, "activo") == "t";
    d.notas             = r.val(i, "notas");
    d.created_at        = r.val(i, "created_at");
    d.updated_at        = r.val(i, "updated_at");
    return d;
}

json EmployeeDependent::toJson() const {
    auto nul = [](const std::string& s) -> json { return s.empty() ? json(nullptr) : json(s); };
    return {
        {"id",                id},
        {"employee_id",       employee_id},
        {"tenant_id",         tenant_id},
        {"nombres",           nombres},
        {"apellidos",         apellidos},
        {"nombre_completo",   nombres + " " + apellidos},
        {"fecha_nacimiento",  nul(fecha_nacimiento)},
        {"parentesco",        parentesco},
        {"vive_con_empleado", vive_con_empleado},
        {"activo",            activo},
        {"notas",             nul(notas)},
        {"created_at",        created_at},
        {"updated_at",        updated_at}
    };
}

EmployeeDependent EmployeeDependent::create(const std::string& employee_id,
                                             const std::string& tenant_id,
                                             const json& data) {
    auto str = [&](const char* k, const char* def = "") -> std::string {
        return (data.contains(k) && data[k].is_string()) ? data[k].get<std::string>() : def;
    };
    bool vive = true;
    if (data.contains("vive_con_empleado") && data["vive_con_empleado"].is_boolean())
        vive = data["vive_con_empleado"].get<bool>();

    auto r = PgConnection::getInstance().exec(
        "INSERT INTO public.employee_dependents "
        "(employee_id, tenant_id, nombres, apellidos, fecha_nacimiento, "
        " parentesco, vive_con_empleado, notas) "
        "VALUES ($1,$2,$3,$4,NULLIF($5,'')::DATE,$6,$7,NULLIF($8,'')) "
        "RETURNING *",
        {employee_id, tenant_id,
         str("nombres"), str("apellidos"), str("fecha_nacimiento"),
         str("parentesco"), vive ? "true" : "false", str("notas")}
    );
    if (!r.ok() || r.rows() == 0)
        throw PgException(std::string("EmployeeDependent::create: ") + r.errMsg(), r.pgCode());
    return rowToDependent(r, 0);
}

std::vector<EmployeeDependent> EmployeeDependent::findByEmployee(const std::string& employee_id,
                                                                   const std::string& tenant_id,
                                                                   bool only_active) {
    PgResult r = only_active
        ? PgConnection::getInstance().exec(
            "SELECT * FROM public.employee_dependents "
            "WHERE employee_id=$1 AND tenant_id=$2 AND activo=TRUE "
            "ORDER BY parentesco, nombres",
            {employee_id, tenant_id})
        : PgConnection::getInstance().exec(
            "SELECT * FROM public.employee_dependents "
            "WHERE employee_id=$1 AND tenant_id=$2 ORDER BY parentesco, nombres",
            {employee_id, tenant_id});

    std::vector<EmployeeDependent> out;
    if (!r.ok()) return out;
    for (int i = 0; i < r.rows(); ++i) out.push_back(rowToDependent(r, i));
    return out;
}

bool EmployeeDependent::deactivate(const std::string& id,
                                    const std::string& employee_id,
                                    const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "UPDATE public.employee_dependents SET activo=FALSE "
        "WHERE id=$1 AND employee_id=$2 AND tenant_id=$3 RETURNING id",
        {id, employee_id, tenant_id});
    return r.cmdOk() || (r.ok() && r.rows() > 0);
}
