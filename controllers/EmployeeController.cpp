#include "EmployeeController.h"
#include "../models/rrhh/Employee.h"
#include "../utils/JwtUtils.h"
#include <nlohmann/json.hpp>
#include <set>

using json = nlohmann::json;

static HttpResponsePtr jsonResp(HttpStatusCode code, const json& body) {
    auto resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(code);
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    resp->setBody(body.dump());
    return resp;
}

static const std::set<std::string> VALID_DEPTS = {
    "comercial","proyectos","produccion","almacen","rrhh","gerencia","administrativo"
};
static const std::set<std::string> VALID_DOCS  = {"CC","CE","PA","TI","NIT"};
static const std::set<std::string> VALID_CONTRATOS = {
    "indefinido","fijo","obra_labor","prestacion_servicios","aprendiz"
};

// ── POST /api/v1/rrhh/employees ───────────────────────────────────────────────
void EmployeeController::createEmployee(const HttpRequestPtr& req,
                                         std::function<void(const HttpResponsePtr&)>&& cb) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    json body;
    try { body = json::parse(req->getBody()); }
    catch (...) { cb(jsonResp(k400BadRequest, {{"error","JSON mal formado"}})); return; }

    // Validaciones requeridas
    auto reqStr = [&](const char* f) {
        return body.contains(f) && body[f].is_string() && !body[f].get<std::string>().empty();
    };
    if (!reqStr("nombres"))          { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: nombres"}}));         return; }
    if (!reqStr("apellidos"))        { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: apellidos"}}));       return; }
    if (!reqStr("tipo_documento"))   { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: tipo_documento"}}));  return; }
    if (!reqStr("numero_documento")) { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: numero_documento"}}));return; }
    if (!reqStr("cargo"))            { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: cargo"}}));           return; }
    if (!reqStr("departamento"))     { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: departamento"}}));    return; }
    if (!reqStr("tipo_contrato"))    { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: tipo_contrato"}}));   return; }
    if (!reqStr("fecha_ingreso"))    { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: fecha_ingreso"}}));   return; }

    if (!VALID_DOCS.count(body["tipo_documento"].get<std::string>()))
        { cb(jsonResp(k400BadRequest, {{"error","tipo_documento inválido. Valores: CC,CE,PA,TI,NIT"}})); return; }
    if (!VALID_DEPTS.count(body["departamento"].get<std::string>()))
        { cb(jsonResp(k400BadRequest, {{"error","departamento inválido"}})); return; }
    if (!VALID_CONTRATOS.count(body["tipo_contrato"].get<std::string>()))
        { cb(jsonResp(k400BadRequest, {{"error","tipo_contrato inválido"}})); return; }

    try {
        auto emp = Employee::create(claims->tenant_id, body);
        cb(jsonResp(k201Created, {{"employee", emp.toJson()}}));
    } catch (const std::exception& e) {
        std::string msg = e.what();
        if (msg.find("employees_doc_tenant") != std::string::npos)
            cb(jsonResp(k409Conflict, {{"error","Ya existe un empleado con ese documento"}}));
        else
            cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// ── GET /api/v1/rrhh/employees ────────────────────────────────────────────────
void EmployeeController::listEmployees(const HttpRequestPtr& req,
                                        std::function<void(const HttpResponsePtr&)>&& cb) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    std::string dept = req->getParameter("departamento");
    try {
        auto employees = Employee::findAll(claims->tenant_id, dept);
        json arr = json::array();
        for (const auto& e : employees) arr.push_back(e.toJson());
        cb(jsonResp(k200OK, {{"employees", arr}, {"total", arr.size()}}));
    } catch (const std::exception&) {
        cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// ── GET /api/v1/rrhh/employees/{id} ──────────────────────────────────────────
void EmployeeController::getEmployee(const HttpRequestPtr& req,
                                      std::function<void(const HttpResponsePtr&)>&& cb,
                                      std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        auto emp = Employee::findById(id, claims->tenant_id);
        if (!emp) { cb(jsonResp(k404NotFound, {{"error","Empleado no encontrado"}})); return; }

        // Proyectos activos del empleado
        auto proj = PgConnection::getInstance().exec(
            "SELECT pm.rol_proyecto, p.id, p.nombre, p.estado, p.client_id "
            "FROM public.project_members pm "
            "JOIN public.projects p ON p.id = pm.project_id "
            "WHERE pm.employee_id=$1 AND pm.activo=TRUE "
            "ORDER BY p.created_at DESC",
            {id}
        );
        json projArr = json::array();
        for (int i = 0; i < proj.rows(); ++i) {
            projArr.push_back({
                {"project_id",   proj.val(i,"id")},
                {"nombre",       proj.val(i,"nombre")},
                {"estado",       proj.val(i,"estado")},
                {"rol_proyecto", proj.val(i,"rol_proyecto")}
            });
        }

        json body = emp->toJson();
        body["proyectos_activos"] = projArr;
        cb(jsonResp(k200OK, {{"employee", body}}));
    } catch (const std::exception&) {
        cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}
