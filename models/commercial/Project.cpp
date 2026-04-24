#include "Project.h"
#include <libpq-fe.h>
#include <stdexcept>

static Project rowToProject(const PgResult& r, int row) {
    Project p;
    p.id                  = r.val(row, "id");
    p.tenant_id           = r.val(row, "tenant_id");
    p.client_id           = r.val(row, "client_id");
    p.commercial_id       = r.val(row, "commercial_id");
    p.nombre              = r.val(row, "nombre");
    p.descripcion         = r.val(row, "descripcion");
    p.estado              = r.val(row, "estado");
    p.fecha_prometida     = r.val(row, "fecha_prometida");
    p.presupuesto_cliente = r.val(row, "presupuesto_cliente");
    p.notas               = r.val(row, "notas");
    p.created_at          = r.val(row, "created_at");
    p.updated_at          = r.val(row, "updated_at");
    return p;
}

json Project::toJson() const {
    auto nullable = [](const std::string& s) -> json {
        return s.empty() ? json(nullptr) : json(s);
    };
    json j = {
        {"id",            id},
        {"tenant_id",     tenant_id},
        {"client_id",     client_id},
        {"commercial_id", commercial_id},
        {"nombre",        nombre},
        {"descripcion",   nullable(descripcion)},
        {"estado",        estado},
        {"fecha_prometida", nullable(fecha_prometida)},
        {"notas",         nullable(notas)},
        {"created_at",    created_at},
        {"updated_at",    updated_at}
    };
    if (presupuesto_cliente.empty())
        j["presupuesto_cliente"] = nullptr;
    else
        j["presupuesto_cliente"] = std::stod(presupuesto_cliente);
    return j;
}

Project Project::create(const std::string& tenant_id,
                         const std::string& commercial_id, const json& data) {
    auto str = [&](const char* key, const char* def = "") -> std::string {
        if (data.contains(key) && data[key].is_string()) return data[key].get<std::string>();
        return def;
    };
    std::string presupuesto = "";
    if (data.contains("presupuesto_cliente") && data["presupuesto_cliente"].is_number())
        presupuesto = std::to_string(data["presupuesto_cliente"].get<double>());

    auto r = PgConnection::getInstance().exec(
        "INSERT INTO public.projects "
        "(tenant_id,client_id,commercial_id,nombre,descripcion,fecha_prometida,presupuesto_cliente,notas) "
        "VALUES ($1,$2,$3,$4,$5,NULLIF($6,'')::DATE,NULLIF($7,'')::NUMERIC,NULLIF($8,'')) "
        "RETURNING *",
        {tenant_id, str("client_id"), commercial_id,
         str("nombre"), str("descripcion"),
         str("fecha_prometida"), presupuesto, str("notas")}
    );

    if (!r.ok() || r.rows() == 0)
        throw std::runtime_error(std::string("Project::create: ") + r.errMsg());
    return rowToProject(r, 0);
}

std::optional<Project> Project::findById(const std::string& id, const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "SELECT * FROM public.projects WHERE id=$1 AND tenant_id=$2 LIMIT 1", {id, tenant_id});
    if (!r.ok() || r.rows() == 0) return std::nullopt;
    return rowToProject(r, 0);
}

std::vector<Project> Project::findAll(const std::string& tenant_id, const std::string& client_id) {
    PgResult r = client_id.empty()
        ? PgConnection::getInstance().exec(
            "SELECT * FROM public.projects WHERE tenant_id=$1 ORDER BY created_at DESC", {tenant_id})
        : PgConnection::getInstance().exec(
            "SELECT * FROM public.projects WHERE tenant_id=$1 AND client_id=$2 ORDER BY created_at DESC",
            {tenant_id, client_id});

    std::vector<Project> out;
    if (!r.ok()) return out;
    for (int i = 0; i < r.rows(); ++i) out.push_back(rowToProject(r, i));
    return out;
}

void Project::submitForApproval(const std::string& project_id, const std::string& tenant_id) {
    // Verifica que el proyecto esté en borrador
    auto r = PgConnection::getInstance().exec(
        "SELECT estado FROM public.projects WHERE id=$1 AND tenant_id=$2", {project_id, tenant_id});
    if (!r.ok() || r.rows() == 0)
        throw std::runtime_error("Proyecto no encontrado");
    if (r.val(0, "estado") != "borrador")
        throw std::runtime_error("Solo proyectos en estado 'borrador' pueden enviarse a revisión");

    // Crea (o resetea) los 4 registros de aprobación
    PgConnection::getInstance().exec(
        "INSERT INTO public.project_approvals (project_id, departamento) "
        "VALUES ($1,'produccion'),($1,'proyectos'),($1,'almacen'),($1,'gerencia') "
        "ON CONFLICT (project_id, departamento) DO UPDATE "
        "SET estado='pendiente', approver_id=NULL, comentarios=NULL, fecha_resolucion=NULL",
        {project_id}
    );

    PgConnection::getInstance().exec(
        "UPDATE public.projects SET estado='en_revision' WHERE id=$1", {project_id});
}
