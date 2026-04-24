#include "EmployeeVacation.h"
#include <stdexcept>

static EmployeeVacation rowToVacation(const PgResult& r, int i) {
    EmployeeVacation v;
    v.id               = r.val(i, "id");
    v.employee_id      = r.val(i, "employee_id");
    v.tenant_id        = r.val(i, "tenant_id");
    const std::string yr = r.val(i, "periodo_year");
    v.periodo_year     = yr.empty() ? 0 : std::stoi(yr);
    v.dias_disponibles = r.val(i, "dias_disponibles");
    v.dias_tomados     = r.val(i, "dias_tomados");
    v.fecha_inicio     = r.val(i, "fecha_inicio");
    v.fecha_fin        = r.val(i, "fecha_fin");
    v.estado           = r.val(i, "estado");
    v.aprobado_por     = r.val(i, "aprobado_por");
    v.notas            = r.val(i, "notas");
    v.created_at       = r.val(i, "created_at");
    v.updated_at       = r.val(i, "updated_at");
    return v;
}

json EmployeeVacation::toJson() const {
    auto nul = [](const std::string& s) -> json { return s.empty() ? json(nullptr) : json(s); };
    auto num = [](const std::string& s) -> json {
        if (s.empty()) return json(nullptr);
        try { return json(std::stod(s)); } catch (...) { return json(nullptr); }
    };
    return {
        {"id",               id},
        {"employee_id",      employee_id},
        {"tenant_id",        tenant_id},
        {"periodo_year",     periodo_year},
        {"dias_disponibles", num(dias_disponibles)},
        {"dias_tomados",     num(dias_tomados)},
        {"dias_pendientes",  (dias_disponibles.empty() || dias_tomados.empty())
                                ? json(nullptr)
                                : json(std::stod(dias_disponibles) - std::stod(dias_tomados))},
        {"fecha_inicio",     nul(fecha_inicio)},
        {"fecha_fin",        nul(fecha_fin)},
        {"estado",           estado},
        {"aprobado_por",     nul(aprobado_por)},
        {"notas",            nul(notas)},
        {"created_at",       created_at},
        {"updated_at",       updated_at}
    };
}

EmployeeVacation EmployeeVacation::ensurePeriod(const std::string& employee_id,
                                                  const std::string& tenant_id,
                                                  int year,
                                                  double dias_disponibles) {
    auto r = PgConnection::getInstance().exec(
        "INSERT INTO public.employee_vacations "
        "(employee_id, tenant_id, periodo_year, dias_disponibles) "
        "VALUES ($1,$2,$3,$4) "
        "ON CONFLICT (employee_id, periodo_year) DO UPDATE "
        "SET updated_at=now() "
        "RETURNING *",
        {employee_id, tenant_id, std::to_string(year), std::to_string(dias_disponibles)}
    );
    if (!r.ok() || r.rows() == 0)
        throw PgException(std::string("EmployeeVacation::ensurePeriod: ") + r.errMsg(), r.pgCode());
    return rowToVacation(r, 0);
}

std::vector<EmployeeVacation> EmployeeVacation::findByEmployee(const std::string& employee_id,
                                                                 const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "SELECT * FROM public.employee_vacations "
        "WHERE employee_id=$1 AND tenant_id=$2 ORDER BY periodo_year DESC",
        {employee_id, tenant_id});
    std::vector<EmployeeVacation> out;
    if (!r.ok()) return out;
    for (int i = 0; i < r.rows(); ++i) out.push_back(rowToVacation(r, i));
    return out;
}

std::optional<EmployeeVacation> EmployeeVacation::update(const std::string& id,
                                                           const std::string& employee_id,
                                                           const std::string& tenant_id,
                                                           const json& data,
                                                           const std::string& aprobado_por) {
    auto str = [&](const char* k) -> std::string {
        return (data.contains(k) && data[k].is_string()) ? data[k].get<std::string>() : "";
    };
    std::string dias_tomados_s;
    if (data.contains("dias_tomados") && data["dias_tomados"].is_number())
        dias_tomados_s = std::to_string(data["dias_tomados"].get<double>());

    auto r = PgConnection::getInstance().exec(
        "UPDATE public.employee_vacations SET "
        "  dias_tomados = CASE WHEN $4='' THEN dias_tomados ELSE $4::NUMERIC END, "
        "  fecha_inicio = CASE WHEN $5='' THEN fecha_inicio ELSE $5::DATE END, "
        "  fecha_fin    = CASE WHEN $6='' THEN fecha_fin    ELSE $6::DATE END, "
        "  estado       = CASE WHEN $7='' THEN estado       ELSE $7 END, "
        "  aprobado_por = CASE WHEN $8='' THEN aprobado_por ELSE $8::UUID END, "
        "  notas        = CASE WHEN $9='' THEN notas        ELSE NULLIF($9,'') END "
        "WHERE id=$1 AND employee_id=$2 AND tenant_id=$3 RETURNING *",
        {id, employee_id, tenant_id,
         dias_tomados_s, str("fecha_inicio"), str("fecha_fin"),
         str("estado"), aprobado_por, str("notas")}
    );
    if (!r.ok() || r.rows() == 0) return std::nullopt;
    return rowToVacation(r, 0);
}

bool EmployeeVacation::remove(const std::string& id, const std::string& employee_id, const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "DELETE FROM public.employee_vacations "
        "WHERE id=$1 AND employee_id=$2 AND tenant_id=$3",
        {id, employee_id, tenant_id});
    return r.ok();
}
