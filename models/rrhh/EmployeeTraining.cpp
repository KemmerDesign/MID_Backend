#include "EmployeeTraining.h"
#include <stdexcept>

static EmployeeTraining rowToTraining(const PgResult& r, int i) {
    EmployeeTraining t;
    t.id               = r.val(i, "id");
    t.employee_id      = r.val(i, "employee_id");
    t.tenant_id        = r.val(i, "tenant_id");
    t.nombre_curso     = r.val(i, "nombre_curso");
    t.institucion      = r.val(i, "institucion");
    t.tipo             = r.val(i, "tipo");
    t.modalidad        = r.val(i, "modalidad");
    t.fecha_inicio     = r.val(i, "fecha_inicio");
    t.fecha_fin        = r.val(i, "fecha_fin");
    t.horas            = r.val(i, "horas");
    t.resultado        = r.val(i, "resultado");
    t.fecha_vencimiento = r.val(i, "fecha_vencimiento");
    t.certificado_url  = r.val(i, "certificado_url");
    t.notas            = r.val(i, "notas");
    t.created_at       = r.val(i, "created_at");
    t.updated_at       = r.val(i, "updated_at");
    return t;
}

json EmployeeTraining::toJson() const {
    auto nul = [](const std::string& s) -> json { return s.empty() ? json(nullptr) : json(s); };
    auto intVal = [](const std::string& s) -> json {
        if (s.empty()) return json(nullptr);
        try { return json(std::stoi(s)); } catch (...) { return json(nullptr); }
    };
    return {
        {"id",               id},
        {"employee_id",      employee_id},
        {"tenant_id",        tenant_id},
        {"nombre_curso",     nombre_curso},
        {"institucion",      nul(institucion)},
        {"tipo",             tipo},
        {"modalidad",        nul(modalidad)},
        {"fecha_inicio",     nul(fecha_inicio)},
        {"fecha_fin",        nul(fecha_fin)},
        {"horas",            intVal(horas)},
        {"resultado",        nul(resultado)},
        {"fecha_vencimiento",nul(fecha_vencimiento)},
        {"certificado_url",  nul(certificado_url)},
        {"notas",            nul(notas)},
        {"created_at",       created_at},
        {"updated_at",       updated_at}
    };
}

EmployeeTraining EmployeeTraining::create(const std::string& employee_id,
                                           const std::string& tenant_id,
                                           const json& data) {
    auto str = [&](const char* k, const char* def = "") -> std::string {
        return (data.contains(k) && data[k].is_string()) ? data[k].get<std::string>() : def;
    };
    std::string horas;
    if (data.contains("horas") && data["horas"].is_number_integer())
        horas = std::to_string(data["horas"].get<int>());

    auto r = PgConnection::getInstance().exec(
        "INSERT INTO public.employee_training "
        "(employee_id, tenant_id, nombre_curso, institucion, tipo, modalidad, "
        " fecha_inicio, fecha_fin, horas, resultado, fecha_vencimiento, certificado_url, notas) "
        "VALUES ($1,$2,$3,NULLIF($4,''),$5,NULLIF($6,''),"
        "        NULLIF($7,'')::DATE,NULLIF($8,'')::DATE,NULLIF($9,'')::INTEGER,"
        "        NULLIF($10,''),NULLIF($11,'')::DATE,NULLIF($12,''),NULLIF($13,'')) "
        "RETURNING *",
        {employee_id, tenant_id,
         str("nombre_curso"), str("institucion"), str("tipo"), str("modalidad"),
         str("fecha_inicio"), str("fecha_fin"), horas,
         str("resultado"), str("fecha_vencimiento"), str("certificado_url"), str("notas")}
    );
    if (!r.ok() || r.rows() == 0)
        throw PgException(std::string("EmployeeTraining::create: ") + r.errMsg(), r.pgCode());
    return rowToTraining(r, 0);
}

std::vector<EmployeeTraining> EmployeeTraining::findByEmployee(const std::string& employee_id,
                                                                 const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "SELECT * FROM public.employee_training "
        "WHERE employee_id=$1 AND tenant_id=$2 ORDER BY fecha_inicio DESC NULLS LAST",
        {employee_id, tenant_id});
    std::vector<EmployeeTraining> out;
    if (!r.ok()) return out;
    for (int i = 0; i < r.rows(); ++i) out.push_back(rowToTraining(r, i));
    return out;
}

std::optional<EmployeeTraining> EmployeeTraining::update(const std::string& id,
                                                           const std::string& employee_id,
                                                           const std::string& tenant_id,
                                                           const json& data) {
    auto str = [&](const char* k, const char* def = "") -> std::string {
        return (data.contains(k) && data[k].is_string()) ? data[k].get<std::string>() : def;
    };
    std::string horas;
    if (data.contains("horas") && data["horas"].is_number_integer())
        horas = std::to_string(data["horas"].get<int>());

    auto r = PgConnection::getInstance().exec(
        "UPDATE public.employee_training SET "
        "    nombre_curso=COALESCE(NULLIF($3,''), nombre_curso), "
        "    institucion=COALESCE(NULLIF($4,''), institucion), "
        "    tipo=COALESCE(NULLIF($5,''), tipo), "
        "    modalidad=COALESCE(NULLIF($6,''), modalidad), "
        "    fecha_inicio=COALESCE(NULLIF($7,'')::DATE, fecha_inicio), "
        "    fecha_fin=COALESCE(NULLIF($8,'')::DATE, fecha_fin), "
        "    horas=COALESCE(NULLIF($9,'')::INTEGER, horas), "
        "    resultado=COALESCE(NULLIF($10,''), resultado), "
        "    fecha_vencimiento=COALESCE(NULLIF($11,'')::DATE, fecha_vencimiento), "
        "    certificado_url=COALESCE(NULLIF($12,''), certificado_url), "
        "    notas=COALESCE(NULLIF($13,''), notas) "
        "WHERE id=$1 AND employee_id=$2 AND tenant_id=$14 RETURNING *",
        {id, employee_id,
         str("nombre_curso"), str("institucion"), str("tipo"), str("modalidad"),
         str("fecha_inicio"), str("fecha_fin"), horas,
         str("resultado"), str("fecha_vencimiento"), str("certificado_url"), str("notas"),
         tenant_id}
    );
    if (!r.ok() || r.rows() == 0) return std::nullopt;
    return rowToTraining(r, 0);
}

bool EmployeeTraining::remove(const std::string& id,
                               const std::string& employee_id,
                               const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "DELETE FROM public.employee_training "
        "WHERE id=$1 AND employee_id=$2 AND tenant_id=$3 RETURNING id",
        {id, employee_id, tenant_id});
    return r.cmdOk() || (r.ok() && r.rows() > 0);
}
