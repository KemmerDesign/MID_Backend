#include "EmployeePosition.h"
#include <stdexcept>

static EmployeePosition rowToPosition(const PgResult& r, int i) {
    EmployeePosition p;
    p.id            = r.val(i, "id");
    p.employee_id   = r.val(i, "employee_id");
    p.tenant_id     = r.val(i, "tenant_id");
    p.rol_id        = r.val(i, "rol_id");
    p.cargo         = r.val(i, "cargo");
    p.departamento  = r.val(i, "departamento");
    p.salario_base  = r.val(i, "salario_base");
    p.fecha_inicio  = r.val(i, "fecha_inicio");
    p.fecha_fin     = r.val(i, "fecha_fin");
    p.motivo        = r.val(i, "motivo");
    p.notas         = r.val(i, "notas");
    p.registrado_por= r.val(i, "registrado_por");
    p.es_honorarios = r.val(i, "es_honorarios") == "t";
    p.created_at    = r.val(i, "created_at");
    return p;
}

json EmployeePosition::toJson() const {
    auto nul = [](const std::string& s) -> json { return s.empty() ? json(nullptr) : json(s); };
    return {
        {"id",             id},
        {"employee_id",    employee_id},
        {"tenant_id",      tenant_id},
        {"rol_id",         nul(rol_id)},
        {"cargo",          cargo},
        {"departamento",   departamento},
        {"salario_base",   salario_base.empty() ? json(nullptr) : json(std::stod(salario_base))},
        {"fecha_inicio",   fecha_inicio},
        {"fecha_fin",      nul(fecha_fin)},
        {"motivo",         motivo},
        {"notas",          nul(notas)},
        {"registrado_por", nul(registrado_por)},
        {"es_honorarios",  es_honorarios},
        {"created_at",     created_at},
        {"es_actual",      fecha_fin.empty()}
    };
}

EmployeePosition EmployeePosition::applyChange(const std::string& employee_id,
                                                const std::string& tenant_id,
                                                const std::string& registrado_por,
                                                const json& data) {
    auto str = [&](const char* k, const char* def = "") -> std::string {
        return (data.contains(k) && data[k].is_string()) ? data[k].get<std::string>() : def;
    };
    std::string salario;
    if (data.contains("salario_base") && data["salario_base"].is_number())
        salario = std::to_string(data["salario_base"].get<double>());

    std::string fecha_inicio = str("fecha_inicio");
    if (fecha_inicio.empty())
        throw std::runtime_error("EmployeePosition::applyChange: fecha_inicio es requerida");

    // Cerrar el período activo anterior
    PgConnection::getInstance().exec(
        "UPDATE public.employee_positions "
        "SET fecha_fin = $3::DATE - INTERVAL '1 day' "
        "WHERE employee_id=$1 AND tenant_id=$2 AND fecha_fin IS NULL",
        {employee_id, tenant_id, fecha_inicio}
    );

    // Insertar nuevo período
    bool es_honorarios = false;
    if (data.contains("es_honorarios") && data["es_honorarios"].is_boolean())
        es_honorarios = data["es_honorarios"].get<bool>();

    auto r = PgConnection::getInstance().exec(
        "INSERT INTO public.employee_positions "
        "(employee_id, tenant_id, rol_id, cargo, departamento, salario_base, "
        " fecha_inicio, motivo, notas, registrado_por, es_honorarios) "
        "VALUES ($1,$2,NULLIF($3,'')::UUID,$4,$5,$6::NUMERIC,$7::DATE,$8,NULLIF($9,''),$10::UUID,$11) "
        "RETURNING *",
        {employee_id, tenant_id,
         str("rol_id"), str("cargo"), str("departamento"), salario,
         fecha_inicio, str("motivo","otro"), str("notas"), registrado_por,
         es_honorarios ? "true" : "false"}
    );
    if (!r.ok() || r.rows() == 0)
        throw PgException(std::string("EmployeePosition::applyChange: ") + r.errMsg(), r.pgCode());

    auto pos = rowToPosition(r, 0);

    // Actualizar snapshot en employees
    PgConnection::getInstance().exec(
        "UPDATE public.employees "
        "SET cargo=$3, departamento=$4, salario_base=NULLIF($5,'')::NUMERIC "
        "WHERE id=$1 AND tenant_id=$2",
        {employee_id, tenant_id, pos.cargo, pos.departamento, pos.salario_base}
    );

    return pos;
}

std::vector<EmployeePosition> EmployeePosition::findByEmployee(const std::string& employee_id,
                                                                const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "SELECT * FROM public.employee_positions "
        "WHERE employee_id=$1 AND tenant_id=$2 ORDER BY fecha_inicio DESC",
        {employee_id, tenant_id});
    std::vector<EmployeePosition> out;
    if (!r.ok()) return out;
    for (int i = 0; i < r.rows(); ++i) out.push_back(rowToPosition(r, i));
    return out;
}
