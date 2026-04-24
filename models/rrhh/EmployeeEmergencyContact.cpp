#include "EmployeeEmergencyContact.h"
#include <stdexcept>

static EmployeeEmergencyContact rowToContact(const PgResult& r, int i) {
    EmployeeEmergencyContact c;
    c.id          = r.val(i, "id");
    c.employee_id = r.val(i, "employee_id");
    c.tenant_id   = r.val(i, "tenant_id");
    c.nombres     = r.val(i, "nombres");
    c.apellidos   = r.val(i, "apellidos");
    c.parentesco  = r.val(i, "parentesco");
    c.telefono1   = r.val(i, "telefono1");
    c.telefono2   = r.val(i, "telefono2");
    c.email       = r.val(i, "email");
    c.es_principal = r.val(i, "es_principal") == "t";
    c.created_at  = r.val(i, "created_at");
    c.updated_at  = r.val(i, "updated_at");
    return c;
}

json EmployeeEmergencyContact::toJson() const {
    auto nul = [](const std::string& s) -> json { return s.empty() ? json(nullptr) : json(s); };
    return {
        {"id",           id},
        {"employee_id",  employee_id},
        {"tenant_id",    tenant_id},
        {"nombres",      nombres},
        {"apellidos",    apellidos},
        {"nombre_completo", nombres + " " + apellidos},
        {"parentesco",   parentesco},
        {"telefono1",    telefono1},
        {"telefono2",    nul(telefono2)},
        {"email",        nul(email)},
        {"es_principal", es_principal},
        {"created_at",   created_at},
        {"updated_at",   updated_at}
    };
}

EmployeeEmergencyContact EmployeeEmergencyContact::create(const std::string& employee_id,
                                                           const std::string& tenant_id,
                                                           const json& data) {
    auto str = [&](const char* k, const char* def = "") -> std::string {
        return (data.contains(k) && data[k].is_string()) ? data[k].get<std::string>() : def;
    };
    bool principal = false;
    if (data.contains("es_principal") && data["es_principal"].is_boolean())
        principal = data["es_principal"].get<bool>();

    auto r = PgConnection::getInstance().exec(
        "INSERT INTO public.employee_emergency_contacts "
        "(employee_id, tenant_id, nombres, apellidos, parentesco, "
        " telefono1, telefono2, email, es_principal) "
        "VALUES ($1,$2,$3,$4,$5,$6,NULLIF($7,''),NULLIF($8,''),$9) "
        "RETURNING *",
        {employee_id, tenant_id,
         str("nombres"), str("apellidos"), str("parentesco"),
         str("telefono1"), str("telefono2"), str("email"),
         principal ? "true" : "false"}
    );
    if (!r.ok() || r.rows() == 0)
        throw PgException(std::string("EmployeeEmergencyContact::create: ") + r.errMsg(), r.pgCode());
    return rowToContact(r, 0);
}

std::vector<EmployeeEmergencyContact> EmployeeEmergencyContact::findByEmployee(
        const std::string& employee_id, const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "SELECT * FROM public.employee_emergency_contacts "
        "WHERE employee_id=$1 AND tenant_id=$2 ORDER BY es_principal DESC, nombres",
        {employee_id, tenant_id});
    std::vector<EmployeeEmergencyContact> out;
    if (!r.ok()) return out;
    for (int i = 0; i < r.rows(); ++i) out.push_back(rowToContact(r, i));
    return out;
}

bool EmployeeEmergencyContact::remove(const std::string& id,
                                       const std::string& employee_id,
                                       const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "DELETE FROM public.employee_emergency_contacts "
        "WHERE id=$1 AND employee_id=$2 AND tenant_id=$3 RETURNING id",
        {id, employee_id, tenant_id});
    return r.cmdOk() || (r.ok() && r.rows() > 0);
}
