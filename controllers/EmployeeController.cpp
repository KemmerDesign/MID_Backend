#include "EmployeeController.h"
#include "../models/rrhh/Employee.h"
#include "../models/rrhh/EmployeePosition.h"
#include "../models/rrhh/EmployeeDependent.h"
#include "../models/rrhh/EmployeeAbsence.h"
#include "../models/rrhh/EmployeeVacation.h"
#include "../models/rrhh/EmployeeDisciplinary.h"
#include "../models/rrhh/EmployeeEmergencyContact.h"
#include "../models/rrhh/EmployeeDocument.h"
#include "../models/rrhh/EmployeeTraining.h"
#include "../models/rrhh/EmployeeEducation.h"
#include "../models/rrhh/EmployeeWorkHistory.h"
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

// Construye el body de error incluyendo pg_code si la excepción es un PgException.
// Uso: cb(jsonResp(k409Conflict, pgErrBody(e, "Mensaje amigable")));
static json pgErrBody(const std::exception& e, const std::string& msg = "Error interno del servidor") {
    json body = {{"error", msg}};
    if (const auto* pe = dynamic_cast<const PgException*>(&e))
        if (!pe->sqlstate.empty()) body["pg_code"] = pe->sqlstate;
    return body;
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
            cb(jsonResp(k409Conflict, pgErrBody(e, "Ya existe un empleado con ese documento")));
        else
            cb(jsonResp(k500InternalServerError, pgErrBody(e)));
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
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
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
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// ─── Helpers de validación ────────────────────────────────────────────────────

static const std::set<std::string> VALID_DOCUMENT_TIPOS = {
    "cedula","contrato","hoja_vida","diploma","acta_grado","certificado_estudios",
    "certificado_laboral","eps","arl","pension","caja_compensacion","libreta_militar",
    "paz_salvo","acuerdo_confidencialidad","reglamento_interno","otro"
};
static const std::set<std::string> VALID_DOCUMENT_ESTADOS = {
    "pendiente","recibido","vencido","no_aplica"
};
static const std::set<std::string> VALID_TRAINING_TIPOS = {
    "induccion","seguridad_salud","tecnico","habilidades_blandas","certificacion","otro"
};
static const std::set<std::string> VALID_TRAINING_MODALIDADES = {
    "presencial","virtual","mixta","e_learning"
};
static const std::set<std::string> VALID_EDUCATION_NIVELES = {
    "bachillerato","tecnico","tecnologo","universitario","especializacion","maestria","doctorado","otro"
};
static const std::set<std::string> VALID_EMERGENCY_PARENTESCOS = {
    "conyuge","companero_permanente","padre","madre","hijo","hija","hermano","hermana","otro"
};

static const std::set<std::string> VALID_ABSENCE_TIPOS = {
    "incapacidad_medica","accidente_trabajo","licencia_maternidad","licencia_paternidad",
    "licencia_luto","permiso_remunerado","permiso_no_remunerado",
    "calamidad_domestica","suspension_disciplinaria","inasistencia_injustificada","otro"
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
            cb(jsonResp(k500InternalServerError, pgErrBody(e)));
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
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
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
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
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
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
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
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
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
            cb(jsonResp(k500InternalServerError, pgErrBody(e)));
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
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
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
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
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
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
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
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
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
            cb(jsonResp(k500InternalServerError, pgErrBody(e)));
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
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
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
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
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
            cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// ─── Contactos de emergencia ──────────────────────────────────────────────────

// POST /api/v1/rrhh/employees/{id}/emergency-contacts
void EmployeeController::addEmergencyContact(const HttpRequestPtr& req,
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
    if (!reqStr("nombres"))   { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: nombres"}}));   return; }
    if (!reqStr("apellidos")) { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: apellidos"}})); return; }
    if (!reqStr("telefono1")) { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: telefono1"}})); return; }
    if (!reqStr("parentesco") || !VALID_EMERGENCY_PARENTESCOS.count(body["parentesco"].get<std::string>()))
        { cb(jsonResp(k400BadRequest, {{"error","parentesco inválido o ausente"}})); return; }

    try {
        auto c = EmployeeEmergencyContact::create(id, claims->tenant_id, body);
        cb(jsonResp(k201Created, {{"emergency_contact", c.toJson()}}));
    } catch (const std::exception& e) {
        std::string msg = e.what();
        if (msg.find("uq_emergency_principal") != std::string::npos)
            cb(jsonResp(k409Conflict, pgErrBody(e, "Ya existe un contacto principal; desactívalo antes")));
        else
            cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// GET /api/v1/rrhh/employees/{id}/emergency-contacts
void EmployeeController::listEmergencyContacts(const HttpRequestPtr& req,
                                                std::function<void(const HttpResponsePtr&)>&& cb,
                                                std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        auto contacts = EmployeeEmergencyContact::findByEmployee(id, claims->tenant_id);
        json arr = json::array();
        for (const auto& c : contacts) arr.push_back(c.toJson());
        cb(jsonResp(k200OK, {{"emergency_contacts", arr}, {"total", arr.size()}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// DELETE /api/v1/rrhh/employees/{id}/emergency-contacts/{contact_id}
void EmployeeController::removeEmergencyContact(const HttpRequestPtr& req,
                                                 std::function<void(const HttpResponsePtr&)>&& cb,
                                                 std::string&& id, std::string&& contact_id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        bool ok = EmployeeEmergencyContact::remove(contact_id, id, claims->tenant_id);
        if (!ok) { cb(jsonResp(k404NotFound, {{"error","Contacto no encontrado"}})); return; }
        cb(jsonResp(k200OK, {{"message","Contacto de emergencia eliminado"}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// ─── Documentos ───────────────────────────────────────────────────────────────

// POST /api/v1/rrhh/employees/{id}/documents
void EmployeeController::addDocument(const HttpRequestPtr& req,
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
    if (!reqStr("tipo") || !VALID_DOCUMENT_TIPOS.count(body["tipo"].get<std::string>()))
        { cb(jsonResp(k400BadRequest, {{"error","tipo de documento inválido o ausente"}})); return; }
    if (!reqStr("nombre")) { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: nombre"}})); return; }

    try {
        auto doc = EmployeeDocument::create(id, claims->tenant_id, body);
        cb(jsonResp(k201Created, {{"document", doc.toJson()}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// GET /api/v1/rrhh/employees/{id}/documents
void EmployeeController::listDocuments(const HttpRequestPtr& req,
                                        std::function<void(const HttpResponsePtr&)>&& cb,
                                        std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        auto docs = EmployeeDocument::findByEmployee(id, claims->tenant_id);
        json arr = json::array();
        for (const auto& d : docs) arr.push_back(d.toJson());
        cb(jsonResp(k200OK, {{"documents", arr}, {"total", arr.size()}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// PATCH /api/v1/rrhh/employees/{id}/documents/{doc_id}
void EmployeeController::updateDocument(const HttpRequestPtr& req,
                                         std::function<void(const HttpResponsePtr&)>&& cb,
                                         std::string&& id, std::string&& doc_id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    json body;
    try { body = json::parse(req->getBody()); }
    catch (...) { cb(jsonResp(k400BadRequest, {{"error","JSON mal formado"}})); return; }

    if (body.contains("estado") && !VALID_DOCUMENT_ESTADOS.count(body["estado"].get<std::string>()))
        { cb(jsonResp(k400BadRequest, {{"error","estado inválido"}})); return; }

    try {
        auto doc = EmployeeDocument::updateStatus(doc_id, id, claims->tenant_id, body);
        if (!doc) { cb(jsonResp(k404NotFound, {{"error","Documento no encontrado"}})); return; }
        cb(jsonResp(k200OK, {{"document", doc->toJson()}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// DELETE /api/v1/rrhh/employees/{id}/documents/{doc_id}
void EmployeeController::removeDocument(const HttpRequestPtr& req,
                                         std::function<void(const HttpResponsePtr&)>&& cb,
                                         std::string&& id, std::string&& doc_id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        bool ok = EmployeeDocument::remove(doc_id, id, claims->tenant_id);
        if (!ok) { cb(jsonResp(k404NotFound, {{"error","Documento no encontrado"}})); return; }
        cb(jsonResp(k200OK, {{"message","Documento eliminado"}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// ─── Capacitaciones ───────────────────────────────────────────────────────────

// POST /api/v1/rrhh/employees/{id}/training
void EmployeeController::addTraining(const HttpRequestPtr& req,
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
    if (!reqStr("nombre_curso")) { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: nombre_curso"}})); return; }
    if (!reqStr("tipo") || !VALID_TRAINING_TIPOS.count(body["tipo"].get<std::string>()))
        { cb(jsonResp(k400BadRequest, {{"error","tipo inválido o ausente"}})); return; }
    if (reqStr("modalidad") && !VALID_TRAINING_MODALIDADES.count(body["modalidad"].get<std::string>()))
        { cb(jsonResp(k400BadRequest, {{"error","modalidad inválida"}})); return; }

    try {
        auto t = EmployeeTraining::create(id, claims->tenant_id, body);
        cb(jsonResp(k201Created, {{"training", t.toJson()}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// GET /api/v1/rrhh/employees/{id}/training
void EmployeeController::listTraining(const HttpRequestPtr& req,
                                       std::function<void(const HttpResponsePtr&)>&& cb,
                                       std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        auto items = EmployeeTraining::findByEmployee(id, claims->tenant_id);
        json arr = json::array();
        for (const auto& t : items) arr.push_back(t.toJson());
        cb(jsonResp(k200OK, {{"training", arr}, {"total", arr.size()}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// PATCH /api/v1/rrhh/employees/{id}/training/{training_id}
void EmployeeController::updateTraining(const HttpRequestPtr& req,
                                         std::function<void(const HttpResponsePtr&)>&& cb,
                                         std::string&& id, std::string&& training_id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    json body;
    try { body = json::parse(req->getBody()); }
    catch (...) { cb(jsonResp(k400BadRequest, {{"error","JSON mal formado"}})); return; }

    if (body.contains("tipo") && !VALID_TRAINING_TIPOS.count(body["tipo"].get<std::string>()))
        { cb(jsonResp(k400BadRequest, {{"error","tipo inválido"}})); return; }
    if (body.contains("modalidad") && !VALID_TRAINING_MODALIDADES.count(body["modalidad"].get<std::string>()))
        { cb(jsonResp(k400BadRequest, {{"error","modalidad inválida"}})); return; }

    try {
        auto t = EmployeeTraining::update(training_id, id, claims->tenant_id, body);
        if (!t) { cb(jsonResp(k404NotFound, {{"error","Capacitación no encontrada"}})); return; }
        cb(jsonResp(k200OK, {{"training", t->toJson()}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// DELETE /api/v1/rrhh/employees/{id}/training/{training_id}
void EmployeeController::removeTraining(const HttpRequestPtr& req,
                                         std::function<void(const HttpResponsePtr&)>&& cb,
                                         std::string&& id, std::string&& training_id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        bool ok = EmployeeTraining::remove(training_id, id, claims->tenant_id);
        if (!ok) { cb(jsonResp(k404NotFound, {{"error","Capacitación no encontrada"}})); return; }
        cb(jsonResp(k200OK, {{"message","Capacitación eliminada"}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// ─── Educación ────────────────────────────────────────────────────────────────

// POST /api/v1/rrhh/employees/{id}/education
void EmployeeController::addEducation(const HttpRequestPtr& req,
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
    if (!reqStr("nivel") || !VALID_EDUCATION_NIVELES.count(body["nivel"].get<std::string>()))
        { cb(jsonResp(k400BadRequest, {{"error","nivel educativo inválido o ausente"}})); return; }
    if (!reqStr("titulo"))      { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: titulo"}}));      return; }
    if (!reqStr("institucion")) { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: institucion"}})); return; }

    try {
        auto edu = EmployeeEducation::create(id, claims->tenant_id, body);
        cb(jsonResp(k201Created, {{"education", edu.toJson()}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// GET /api/v1/rrhh/employees/{id}/education
void EmployeeController::listEducation(const HttpRequestPtr& req,
                                        std::function<void(const HttpResponsePtr&)>&& cb,
                                        std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        auto items = EmployeeEducation::findByEmployee(id, claims->tenant_id);
        json arr = json::array();
        for (const auto& e : items) arr.push_back(e.toJson());
        cb(jsonResp(k200OK, {{"education", arr}, {"total", arr.size()}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// DELETE /api/v1/rrhh/employees/{id}/education/{edu_id}
void EmployeeController::removeEducation(const HttpRequestPtr& req,
                                          std::function<void(const HttpResponsePtr&)>&& cb,
                                          std::string&& id, std::string&& edu_id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        bool ok = EmployeeEducation::remove(edu_id, id, claims->tenant_id);
        if (!ok) { cb(jsonResp(k404NotFound, {{"error","Registro educativo no encontrado"}})); return; }
        cb(jsonResp(k200OK, {{"message","Registro educativo eliminado"}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// ─── Experiencia laboral previa ───────────────────────────────────────────────

// POST /api/v1/rrhh/employees/{id}/work-history
void EmployeeController::addWorkHistory(const HttpRequestPtr& req,
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
    if (!reqStr("empresa"))      { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: empresa"}}));      return; }
    if (!reqStr("cargo"))        { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: cargo"}}));        return; }
    if (!reqStr("fecha_inicio")) { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: fecha_inicio"}})); return; }

    try {
        auto wh = EmployeeWorkHistory::create(id, claims->tenant_id, body);
        cb(jsonResp(k201Created, {{"work_history", wh.toJson()}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// GET /api/v1/rrhh/employees/{id}/work-history
void EmployeeController::listWorkHistory(const HttpRequestPtr& req,
                                          std::function<void(const HttpResponsePtr&)>&& cb,
                                          std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        auto items = EmployeeWorkHistory::findByEmployee(id, claims->tenant_id);
        json arr = json::array();
        for (const auto& w : items) arr.push_back(w.toJson());
        cb(jsonResp(k200OK, {{"work_history", arr}, {"total", arr.size()}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// DELETE /api/v1/rrhh/employees/{id}/work-history/{history_id}
void EmployeeController::removeWorkHistory(const HttpRequestPtr& req,
                                            std::function<void(const HttpResponsePtr&)>&& cb,
                                            std::string&& id, std::string&& history_id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        bool ok = EmployeeWorkHistory::remove(history_id, id, claims->tenant_id);
        if (!ok) { cb(jsonResp(k404NotFound, {{"error","Experiencia laboral no encontrada"}})); return; }
        cb(jsonResp(k200OK, {{"message","Experiencia laboral eliminada"}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}

// DELETE /api/v1/rrhh/employees/{id}/vacations/{vac_id}
void EmployeeController::removeVacation(const HttpRequestPtr& req,
                                         std::function<void(const HttpResponsePtr&)>&& cb,
                                         std::string&& id, std::string&& vac_id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        bool ok = EmployeeVacation::remove(vac_id, id, claims->tenant_id);
        if (!ok) { cb(jsonResp(k404NotFound, {{"error","Registro de vacaciones no encontrado"}})); return; }
        cb(jsonResp(k200OK, {{"message","Registro de vacaciones eliminado"}}));
    } catch (const std::exception& e) {
        cb(jsonResp(k500InternalServerError, pgErrBody(e)));
    }
}
