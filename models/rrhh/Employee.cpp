#include "Employee.h"
#include <libpq-fe.h>
#include <stdexcept>

static Employee rowToEmployee(const PgResult& r, int i) {
    Employee e;
    e.id                = r.val(i, "id");
    e.tenant_id         = r.val(i, "tenant_id");
    e.user_id           = r.val(i, "user_id");
    e.nombres           = r.val(i, "nombres");
    e.apellidos         = r.val(i, "apellidos");
    e.tipo_documento    = r.val(i, "tipo_documento");
    e.numero_documento  = r.val(i, "numero_documento");
    e.email_personal    = r.val(i, "email_personal");
    e.email_corporativo = r.val(i, "email_corporativo");
    e.telefono1         = r.val(i, "telefono1");
    e.telefono2         = r.val(i, "telefono2");
    e.fecha_nacimiento  = r.val(i, "fecha_nacimiento");
    e.direccion         = r.val(i, "direccion");
    e.ciudad            = r.val(i, "ciudad");
    e.cargo             = r.val(i, "cargo");
    e.departamento      = r.val(i, "departamento");
    e.tipo_contrato     = r.val(i, "tipo_contrato");
    e.fecha_ingreso     = r.val(i, "fecha_ingreso");
    e.fecha_retiro      = r.val(i, "fecha_retiro");
    e.salario_base      = r.val(i, "salario_base");
    e.activo            = r.val(i, "activo") == "t";
    e.notas             = r.val(i, "notas");
    e.created_at        = r.val(i, "created_at");
    e.updated_at        = r.val(i, "updated_at");
    return e;
}

json Employee::toJson() const {
    auto nul = [](const std::string& s) -> json { return s.empty() ? json(nullptr) : json(s); };
    return {
        {"id",                id},
        {"tenant_id",         tenant_id},
        {"user_id",           nul(user_id)},
        {"nombres",           nombres},
        {"apellidos",         apellidos},
        {"nombre_completo",   fullName()},
        {"tipo_documento",    tipo_documento},
        {"numero_documento",  numero_documento},
        {"email_personal",    nul(email_personal)},
        {"email_corporativo", nul(email_corporativo)},
        {"telefono1",         nul(telefono1)},
        {"telefono2",         nul(telefono2)},
        {"fecha_nacimiento",  nul(fecha_nacimiento)},
        {"direccion",         nul(direccion)},
        {"ciudad",            ciudad},
        {"cargo",             cargo},
        {"departamento",      departamento},
        {"tipo_contrato",     tipo_contrato},
        {"fecha_ingreso",     fecha_ingreso},
        {"fecha_retiro",      nul(fecha_retiro)},
        {"salario_base",      salario_base.empty() ? json(nullptr) : json(std::stod(salario_base))},
        {"activo",            activo},
        {"notas",             nul(notas)},
        {"created_at",        created_at},
        {"updated_at",        updated_at}
    };
}

Employee Employee::create(const std::string& tenant_id, const json& data) {
    auto str = [&](const char* k, const char* def = "") -> std::string {
        return (data.contains(k) && data[k].is_string()) ? data[k].get<std::string>() : def;
    };
    std::string salario = "";
    if (data.contains("salario_base") && data["salario_base"].is_number())
        salario = std::to_string(data["salario_base"].get<double>());

    auto r = PgConnection::getInstance().exec(
        "INSERT INTO public.employees "
        "(tenant_id,user_id,nombres,apellidos,tipo_documento,numero_documento,"
        " email_personal,email_corporativo,telefono1,telefono2,fecha_nacimiento,"
        " direccion,ciudad,cargo,departamento,tipo_contrato,fecha_ingreso,"
        " fecha_retiro,salario_base,notas) "
        "VALUES ($1,NULLIF($2,'')::UUID,$3,$4,$5,$6,"
        "NULLIF($7,''),NULLIF($8,''),NULLIF($9,''),NULLIF($10,''),NULLIF($11,'')::DATE,"
        "NULLIF($12,''),$13,$14,$15,$16,$17::DATE,"
        "NULLIF($18,'')::DATE,NULLIF($19,'')::NUMERIC,NULLIF($20,'')) "
        "RETURNING *",
        {tenant_id, str("user_id"),
         str("nombres"), str("apellidos"),
         str("tipo_documento"), str("numero_documento"),
         str("email_personal"), str("email_corporativo"),
         str("telefono1"), str("telefono2"),
         str("fecha_nacimiento"),
         str("direccion"), str("ciudad", "Bogotá"),
         str("cargo"), str("departamento"),
         str("tipo_contrato"), str("fecha_ingreso"),
         str("fecha_retiro"), salario, str("notas")}
    );

    if (!r.ok() || r.rows() == 0)
        throw std::runtime_error(std::string("Employee::create: ") +
                                 (r.res ? PQresultErrorMessage(r.res) : "sin resultado"));
    return rowToEmployee(r, 0);
}

std::optional<Employee> Employee::findById(const std::string& id, const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "SELECT * FROM public.employees WHERE id=$1 AND tenant_id=$2 LIMIT 1", {id, tenant_id});
    if (!r.ok() || r.rows() == 0) return std::nullopt;
    return rowToEmployee(r, 0);
}

std::vector<Employee> Employee::findAll(const std::string& tenant_id, const std::string& departamento) {
    PgResult r = departamento.empty()
        ? PgConnection::getInstance().exec(
            "SELECT * FROM public.employees WHERE tenant_id=$1 ORDER BY apellidos,nombres", {tenant_id})
        : PgConnection::getInstance().exec(
            "SELECT * FROM public.employees WHERE tenant_id=$1 AND departamento=$2 ORDER BY apellidos,nombres",
            {tenant_id, departamento});
    std::vector<Employee> out;
    if (!r.ok()) return out;
    for (int i = 0; i < r.rows(); ++i) out.push_back(rowToEmployee(r, i));
    return out;
}
