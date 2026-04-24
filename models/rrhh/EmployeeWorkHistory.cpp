#include "EmployeeWorkHistory.h"
#include <stdexcept>

static EmployeeWorkHistory rowToWorkHistory(const PgResult& r, int i) {
    EmployeeWorkHistory w;
    w.id            = r.val(i, "id");
    w.employee_id   = r.val(i, "employee_id");
    w.tenant_id     = r.val(i, "tenant_id");
    w.empresa       = r.val(i, "empresa");
    w.cargo         = r.val(i, "cargo");
    w.departamento  = r.val(i, "departamento");
    w.fecha_inicio  = r.val(i, "fecha_inicio");
    w.fecha_fin     = r.val(i, "fecha_fin");
    w.descripcion   = r.val(i, "descripcion");
    w.motivo_retiro = r.val(i, "motivo_retiro");
    w.created_at    = r.val(i, "created_at");
    w.updated_at    = r.val(i, "updated_at");
    return w;
}

json EmployeeWorkHistory::toJson() const {
    auto nul = [](const std::string& s) -> json { return s.empty() ? json(nullptr) : json(s); };
    return {
        {"id",            id},
        {"employee_id",   employee_id},
        {"tenant_id",     tenant_id},
        {"empresa",       empresa},
        {"cargo",         cargo},
        {"departamento",  nul(departamento)},
        {"fecha_inicio",  fecha_inicio},
        {"fecha_fin",     nul(fecha_fin)},
        {"descripcion",   nul(descripcion)},
        {"motivo_retiro", nul(motivo_retiro)},
        {"created_at",    created_at},
        {"updated_at",    updated_at}
    };
}

EmployeeWorkHistory EmployeeWorkHistory::create(const std::string& employee_id,
                                                 const std::string& tenant_id,
                                                 const json& data) {
    auto str = [&](const char* k, const char* def = "") -> std::string {
        return (data.contains(k) && data[k].is_string()) ? data[k].get<std::string>() : def;
    };
    auto r = PgConnection::getInstance().exec(
        "INSERT INTO public.employee_work_history "
        "(employee_id, tenant_id, empresa, cargo, departamento, "
        " fecha_inicio, fecha_fin, descripcion, motivo_retiro) "
        "VALUES ($1,$2,$3,$4,NULLIF($5,''),"
        "        $6::DATE,NULLIF($7,'')::DATE,NULLIF($8,''),NULLIF($9,'')) "
        "RETURNING *",
        {employee_id, tenant_id,
         str("empresa"), str("cargo"), str("departamento"),
         str("fecha_inicio"), str("fecha_fin"),
         str("descripcion"), str("motivo_retiro")}
    );
    if (!r.ok() || r.rows() == 0)
        throw PgException(std::string("EmployeeWorkHistory::create: ") + r.errMsg(), r.pgCode());
    return rowToWorkHistory(r, 0);
}

std::vector<EmployeeWorkHistory> EmployeeWorkHistory::findByEmployee(const std::string& employee_id,
                                                                       const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "SELECT * FROM public.employee_work_history "
        "WHERE employee_id=$1 AND tenant_id=$2 ORDER BY fecha_inicio DESC",
        {employee_id, tenant_id});
    std::vector<EmployeeWorkHistory> out;
    if (!r.ok()) return out;
    for (int i = 0; i < r.rows(); ++i) out.push_back(rowToWorkHistory(r, i));
    return out;
}

bool EmployeeWorkHistory::remove(const std::string& id,
                                  const std::string& employee_id,
                                  const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "DELETE FROM public.employee_work_history "
        "WHERE id=$1 AND employee_id=$2 AND tenant_id=$3 RETURNING id",
        {id, employee_id, tenant_id});
    return r.cmdOk() || (r.ok() && r.rows() > 0);
}
