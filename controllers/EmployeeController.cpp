#include "EmployeeController.h"
#include "../models/rrhh/Employee.h"
#include "../models/rrhh/EmployeePosition.h"
#include "../models/rrhh/EmployeeDependent.h"
#include "../models/rrhh/EmployeeAbsence.h"
#include "../models/rrhh/EmployeeVacation.h"
#include "../models/rrhh/EmployeeDisciplinary.h"
#include "../utils/JwtUtils.h"
#include "../utils/PgConnection.h"
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

// ─── Helpers de validación ────────────────────────────────────────────────────

static const std::set<std::string> VALID_ABSENCE_TIPOS = {
    "incapacidad_medica","accidente_trabajo","licencia_maternidad","licencia_paternidad",
    "licencia_luto","permiso_remunerado","permiso_no_remunerado",
    "calamidad_domestica","suspension_disciplinaria","otro"
};
static const std::set<std::string> VALID_ABSENCE_ESTADOS = {
    "solicitado","aprobado","rechazado","en_curso","completado","cancelado"
};
static const std::set<std::string> VALID_POSITION_MOTIVOS = {
    "contratacion","ascenso","traslado","ajuste_salarial",
    "reclasificacion","cambio_departamento","otro"
};
static const std::set<std::string> VALID_DISCIPLINARY_TIPOS = {
    "llamado_verbal","llamado_escrito","acta_compromiso","suspension","descargo","otro"
};
static const std::set<std::string> VALID_DISCIPLINARY_ESTADOS = {
    "emitido","notificado","firmado_empleado","en_apelacion","cerrado"
};
static const std::set<std::string> VALID_PARENTESCOS = {
    "hijo","hija","conyuge","companero_permanente","otro"
};
static const std::set<std::string> VALID_VAC_ESTADOS = {
    "pendiente","solicitado","aprobado","rechazado","completado","pago_en_dinero"
};

// ─── Historial laboral ────────────────────────────────────────────────────────

// POST /api/v1/rrhh/employees/{id}/position-change
void EmployeeController::applyPositionChange(const HttpRequestPtr& req,
                                              std::function<void(const HttpResponsePtr&)>&& cb,
                                              std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    json body;
    try { body = json::parse(req->getBody()); }
    catch (...) { cb(jsonResp(k400BadRequest, {{"error","JSON mal formado"}})); return; }

    auto reqStr = [&](const char* f) {
        return body.contains(f) && body[f].is_string() && !body[f].get<std::string>().empty();
    };
    if (!reqStr("cargo"))        { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: cargo"}}));        return; }
    if (!reqStr("departamento")) { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: departamento"}})); return; }
    if (!reqStr("fecha_inicio")) { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: fecha_inicio"}})); return; }
    if (!body.contains("salario_base") || !body["salario_base"].is_number())
        { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: salario_base (numérico)"}})); return; }
    if (reqStr("motivo") && !VALID_POSITION_MOTIVOS.count(body["motivo"].get<std::string>()))
        { cb(jsonResp(k400BadRequest, {{"error","motivo inválido"}})); return; }

    try {
        auto pos = EmployeePosition::applyChange(id, claims->tenant_id, claims->user_id, body);
        cb(jsonResp(k201Created, {{"position", pos.toJson()}}));
    } catch (const std::exception& e) {
        std::string msg = e.what();
        if (msg.find("no encontrado") != std::string::npos || msg.find("violates foreign key") != std::string::npos)
            cb(jsonResp(k404NotFound, {{"error","Empleado no encontrado"}}));
        else
            cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// GET /api/v1/rrhh/employees/{id}/positions
void EmployeeController::listPositions(const HttpRequestPtr& req,
                                        std::function<void(const HttpResponsePtr&)>&& cb,
                                        std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        auto positions = EmployeePosition::findByEmployee(id, claims->tenant_id);
        json arr = json::array();
        for (const auto& p : positions) arr.push_back(p.toJson());
        cb(jsonResp(k200OK, {{"positions", arr}, {"total", arr.size()}}));
    } catch (const std::exception&) {
        cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// ─── Dependientes ─────────────────────────────────────────────────────────────

// POST /api/v1/rrhh/employees/{id}/dependents
void EmployeeController::addDependent(const HttpRequestPtr& req,
                                       std::function<void(const HttpResponsePtr&)>&& cb,
                                       std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    json body;
    try { body = json::parse(req->getBody()); }
    catch (...) { cb(jsonResp(k400BadRequest, {{"error","JSON mal formado"}})); return; }

    auto reqStr = [&](const char* f) {
        return body.contains(f) && body[f].is_string() && !body[f].get<std::string>().empty();
    };
    if (!reqStr("nombres"))    { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: nombres"}}));   return; }
    if (!reqStr("apellidos"))  { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: apellidos"}})); return; }
    if (!reqStr("parentesco") || !VALID_PARENTESCOS.count(body["parentesco"].get<std::string>()))
        { cb(jsonResp(k400BadRequest, {{"error","parentesco inválido"}})); return; }

    try {
        auto dep = EmployeeDependent::create(id, claims->tenant_id, body);
        cb(jsonResp(k201Created, {{"dependent", dep.toJson()}}));
    } catch (const std::exception&) {
        cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// GET /api/v1/rrhh/employees/{id}/dependents
void EmployeeController::listDependents(const HttpRequestPtr& req,
                                         std::function<void(const HttpResponsePtr&)>&& cb,
                                         std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    bool all = req->getParameter("all") == "true";
    try {
        auto deps = EmployeeDependent::findByEmployee(id, claims->tenant_id, !all);
        json arr = json::array();
        for (const auto& d : deps) arr.push_back(d.toJson());
        cb(jsonResp(k200OK, {{"dependents", arr}, {"total", arr.size()}}));
    } catch (const std::exception&) {
        cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// DELETE /api/v1/rrhh/employees/{id}/dependents/{dep_id}
void EmployeeController::removeDependent(const HttpRequestPtr& req,
                                          std::function<void(const HttpResponsePtr&)>&& cb,
                                          std::string&& id, std::string&& dep_id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        bool ok = EmployeeDependent::deactivate(dep_id, id, claims->tenant_id);
        if (!ok) { cb(jsonResp(k404NotFound, {{"error","Dependiente no encontrado"}})); return; }
        cb(jsonResp(k200OK, {{"message","Dependiente desvinculado"}}));
    } catch (const std::exception&) {
        cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// ─── Ausencias ────────────────────────────────────────────────────────────────

// POST /api/v1/rrhh/employees/{id}/absences
void EmployeeController::addAbsence(const HttpRequestPtr& req,
                                     std::function<void(const HttpResponsePtr&)>&& cb,
                                     std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    json body;
    try { body = json::parse(req->getBody()); }
    catch (...) { cb(jsonResp(k400BadRequest, {{"error","JSON mal formado"}})); return; }

    auto reqStr = [&](const char* f) {
        return body.contains(f) && body[f].is_string() && !body[f].get<std::string>().empty();
    };
    if (!reqStr("tipo") || !VALID_ABSENCE_TIPOS.count(body["tipo"].get<std::string>()))
        { cb(jsonResp(k400BadRequest, {{"error","tipo inválido o ausente"}})); return; }
    if (!reqStr("fecha_inicio")) { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: fecha_inicio"}})); return; }
    if (!reqStr("fecha_fin"))    { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: fecha_fin"}}));    return; }

    try {
        auto ab = EmployeeAbsence::create(id, claims->tenant_id, claims->user_id, body);
        cb(jsonResp(k201Created, {{"absence", ab.toJson()}}));
    } catch (const std::exception& e) {
        std::string msg = e.what();
        if (msg.find("chk_absence_dates") != std::string::npos)
            cb(jsonResp(k400BadRequest, {{"error","fecha_fin debe ser mayor o igual a fecha_inicio"}}));
        else
            cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// GET /api/v1/rrhh/employees/{id}/absences
void EmployeeController::listAbsences(const HttpRequestPtr& req,
                                       std::function<void(const HttpResponsePtr&)>&& cb,
                                       std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        auto absences = EmployeeAbsence::findByEmployee(id, claims->tenant_id);
        json arr = json::array();
        for (const auto& a : absences) arr.push_back(a.toJson());
        cb(jsonResp(k200OK, {{"absences", arr}, {"total", arr.size()}}));
    } catch (const std::exception&) {
        cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// PATCH /api/v1/rrhh/employees/{id}/absences/{abs_id}
void EmployeeController::updateAbsence(const HttpRequestPtr& req,
                                        std::function<void(const HttpResponsePtr&)>&& cb,
                                        std::string&& id, std::string&& abs_id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    json body;
    try { body = json::parse(req->getBody()); }
    catch (...) { cb(jsonResp(k400BadRequest, {{"error","JSON mal formado"}})); return; }

    if (!body.contains("estado") || !VALID_ABSENCE_ESTADOS.count(body["estado"].get<std::string>()))
        { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: estado (valor inválido)"}})); return; }

    try {
        auto ab = EmployeeAbsence::updateStatus(abs_id, id, claims->tenant_id,
                                                 body["estado"].get<std::string>(),
                                                 claims->user_id);
        if (!ab) { cb(jsonResp(k404NotFound, {{"error","Ausencia no encontrada"}})); return; }
        cb(jsonResp(k200OK, {{"absence", ab->toJson()}}));
    } catch (const std::exception&) {
        cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// ─── Vacaciones ───────────────────────────────────────────────────────────────

// POST /api/v1/rrhh/employees/{id}/vacations
void EmployeeController::ensureVacationPeriod(const HttpRequestPtr& req,
                                               std::function<void(const HttpResponsePtr&)>&& cb,
                                               std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    json body;
    try { body = json::parse(req->getBody()); }
    catch (...) { cb(jsonResp(k400BadRequest, {{"error","JSON mal formado"}})); return; }

    if (!body.contains("periodo_year") || !body["periodo_year"].is_number_integer())
        { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: periodo_year (entero)"}})); return; }

    int year = body["periodo_year"].get<int>();
    double dias = 15.0;
    if (body.contains("dias_disponibles") && body["dias_disponibles"].is_number())
        dias = body["dias_disponibles"].get<double>();

    try {
        auto vac = EmployeeVacation::ensurePeriod(id, claims->tenant_id, year, dias);
        cb(jsonResp(k200OK, {{"vacation_period", vac.toJson()}}));
    } catch (const std::exception&) {
        cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// GET /api/v1/rrhh/employees/{id}/vacations
void EmployeeController::listVacations(const HttpRequestPtr& req,
                                        std::function<void(const HttpResponsePtr&)>&& cb,
                                        std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        auto vacs = EmployeeVacation::findByEmployee(id, claims->tenant_id);
        json arr = json::array();
        for (const auto& v : vacs) arr.push_back(v.toJson());
        cb(jsonResp(k200OK, {{"vacations", arr}, {"total", arr.size()}}));
    } catch (const std::exception&) {
        cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// PATCH /api/v1/rrhh/employees/{id}/vacations/{vac_id}
void EmployeeController::updateVacation(const HttpRequestPtr& req,
                                         std::function<void(const HttpResponsePtr&)>&& cb,
                                         std::string&& id, std::string&& vac_id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    json body;
    try { body = json::parse(req->getBody()); }
    catch (...) { cb(jsonResp(k400BadRequest, {{"error","JSON mal formado"}})); return; }

    if (body.contains("estado") && !VALID_VAC_ESTADOS.count(body["estado"].get<std::string>()))
        { cb(jsonResp(k400BadRequest, {{"error","estado inválido"}})); return; }

    try {
        auto vac = EmployeeVacation::update(vac_id, id, claims->tenant_id, body, claims->user_id);
        if (!vac) { cb(jsonResp(k404NotFound, {{"error","Período vacacional no encontrado"}})); return; }
        cb(jsonResp(k200OK, {{"vacation_period", vac->toJson()}}));
    } catch (const std::exception& e) {
        std::string msg = e.what();
        if (msg.find("chk_vac_days") != std::string::npos)
            cb(jsonResp(k400BadRequest, {{"error","dias_tomados no puede superar dias_disponibles"}}));
        else
            cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// ─── Disciplinario ────────────────────────────────────────────────────────────

// POST /api/v1/rrhh/employees/{id}/disciplinary
void EmployeeController::addDisciplinary(const HttpRequestPtr& req,
                                          std::function<void(const HttpResponsePtr&)>&& cb,
                                          std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    json body;
    try { body = json::parse(req->getBody()); }
    catch (...) { cb(jsonResp(k400BadRequest, {{"error","JSON mal formado"}})); return; }

    auto reqStr = [&](const char* f) {
        return body.contains(f) && body[f].is_string() && !body[f].get<std::string>().empty();
    };
    if (!reqStr("tipo") || !VALID_DISCIPLINARY_TIPOS.count(body["tipo"].get<std::string>()))
        { cb(jsonResp(k400BadRequest, {{"error","tipo inválido o ausente"}})); return; }
    if (!reqStr("fecha"))  { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: fecha"}}));  return; }
    if (!reqStr("motivo")) { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: motivo"}})); return; }

    try {
        auto disc = EmployeeDisciplinary::create(id, claims->tenant_id, claims->user_id, body);
        cb(jsonResp(k201Created, {{"disciplinary", disc.toJson()}}));
    } catch (const std::exception&) {
        cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// GET /api/v1/rrhh/employees/{id}/disciplinary
void EmployeeController::listDisciplinary(const HttpRequestPtr& req,
                                           std::function<void(const HttpResponsePtr&)>&& cb,
                                           std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        auto records = EmployeeDisciplinary::findByEmployee(id, claims->tenant_id);
        json arr = json::array();
        for (const auto& d : records) arr.push_back(d.toJson());
        cb(jsonResp(k200OK, {{"disciplinary", arr}, {"total", arr.size()}}));
    } catch (const std::exception&) {
        cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// PATCH /api/v1/rrhh/employees/{id}/disciplinary/{disc_id}
void EmployeeController::updateDisciplinary(const HttpRequestPtr& req,
                                             std::function<void(const HttpResponsePtr&)>&& cb,
                                             std::string&& id, std::string&& disc_id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    json body;
    try { body = json::parse(req->getBody()); }
    catch (...) { cb(jsonResp(k400BadRequest, {{"error","JSON mal formado"}})); return; }

    if (body.contains("estado") && !VALID_DISCIPLINARY_ESTADOS.count(body["estado"].get<std::string>()))
        { cb(jsonResp(k400BadRequest, {{"error","estado inválido"}})); return; }

    try {
        auto disc = EmployeeDisciplinary::update(disc_id, id, claims->tenant_id, body);
        if (!disc) { cb(jsonResp(k404NotFound, {{"error","Registro disciplinario no encontrado"}})); return; }
        cb(jsonResp(k200OK, {{"disciplinary", disc->toJson()}}));
    } catch (const std::exception& e) {
        std::string msg = e.what();
        if (msg.find("chk_disc_firma") != std::string::npos)
            cb(jsonResp(k400BadRequest, {{"error","Si firmado_por_empleado=true, fecha_firma es requerida"}}));
        else
            cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}
