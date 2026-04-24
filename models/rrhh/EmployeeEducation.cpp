#include "EmployeeEducation.h"
#include <stdexcept>

static EmployeeEducation rowToEducation(const PgResult& r, int i) {
    EmployeeEducation e;
    e.id               = r.val(i, "id");
    e.employee_id      = r.val(i, "employee_id");
    e.tenant_id        = r.val(i, "tenant_id");
    e.nivel            = r.val(i, "nivel");
    e.titulo           = r.val(i, "titulo");
    e.institucion      = r.val(i, "institucion");
    e.anio_inicio      = r.val(i, "anio_inicio");
    e.anio_graduacion  = r.val(i, "anio_graduacion");
    e.en_curso         = r.val(i, "en_curso") == "t";
    e.ciudad           = r.val(i, "ciudad");
    e.notas            = r.val(i, "notas");
    e.created_at       = r.val(i, "created_at");
    e.updated_at       = r.val(i, "updated_at");
    return e;
}

json EmployeeEducation::toJson() const {
    auto nul = [](const std::string& s) -> json { return s.empty() ? json(nullptr) : json(s); };
    auto intVal = [](const std::string& s) -> json {
        if (s.empty()) return json(nullptr);
        try { return json(std::stoi(s)); } catch (...) { return json(nullptr); }
    };
    return {
        {"id",              id},
        {"employee_id",     employee_id},
        {"tenant_id",       tenant_id},
        {"nivel",           nivel},
        {"titulo",          titulo},
        {"institucion",     institucion},
        {"anio_inicio",     intVal(anio_inicio)},
        {"anio_graduacion", intVal(anio_graduacion)},
        {"en_curso",        en_curso},
        {"ciudad",          nul(ciudad)},
        {"notas",           nul(notas)},
        {"created_at",      created_at},
        {"updated_at",      updated_at}
    };
}

EmployeeEducation EmployeeEducation::create(const std::string& employee_id,
                                             const std::string& tenant_id,
                                             const json& data) {
    auto str = [&](const char* k, const char* def = "") -> std::string {
        return (data.contains(k) && data[k].is_string()) ? data[k].get<std::string>() : def;
    };
    bool en_curso = false;
    if (data.contains("en_curso") && data["en_curso"].is_boolean())
        en_curso = data["en_curso"].get<bool>();

    std::string anio_inicio, anio_graduacion;
    if (data.contains("anio_inicio") && data["anio_inicio"].is_number_integer())
        anio_inicio = std::to_string(data["anio_inicio"].get<int>());
    if (data.contains("anio_graduacion") && data["anio_graduacion"].is_number_integer())
        anio_graduacion = std::to_string(data["anio_graduacion"].get<int>());

    auto r = PgConnection::getInstance().exec(
        "INSERT INTO public.employee_education "
        "(employee_id, tenant_id, nivel, titulo, institucion, "
        " anio_inicio, anio_graduacion, en_curso, ciudad, notas) "
        "VALUES ($1,$2,$3,$4,$5,"
        "        NULLIF($6,'')::SMALLINT,NULLIF($7,'')::SMALLINT,$8,NULLIF($9,''),NULLIF($10,'')) "
        "RETURNING *",
        {employee_id, tenant_id,
         str("nivel"), str("titulo"), str("institucion"),
         anio_inicio, anio_graduacion,
         en_curso ? "true" : "false",
         str("ciudad"), str("notas")}
    );
    if (!r.ok() || r.rows() == 0)
        throw std::runtime_error(std::string("EmployeeEducation::create: ") + r.errMsg());
    return rowToEducation(r, 0);
}

std::vector<EmployeeEducation> EmployeeEducation::findByEmployee(const std::string& employee_id,
                                                                   const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "SELECT * FROM public.employee_education "
        "WHERE employee_id=$1 AND tenant_id=$2 ORDER BY anio_inicio DESC NULLS LAST",
        {employee_id, tenant_id});
    std::vector<EmployeeEducation> out;
    if (!r.ok()) return out;
    for (int i = 0; i < r.rows(); ++i) out.push_back(rowToEducation(r, i));
    return out;
}

bool EmployeeEducation::remove(const std::string& id,
                                const std::string& employee_id,
                                const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "DELETE FROM public.employee_education "
        "WHERE id=$1 AND employee_id=$2 AND tenant_id=$3 RETURNING id",
        {id, employee_id, tenant_id});
    return r.cmdOk() || (r.ok() && r.rows() > 0);
}
