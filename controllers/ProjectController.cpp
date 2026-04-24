#include "ProjectController.h"
#include "../models/commercial/Project.h"
#include "../models/commercial/ProductRequest.h"
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

static json approvalRowToJson(const PgResult& r, int i) {
    return {
        {"id",               r.val(i,"id")},
        {"departamento",     r.val(i,"departamento")},
        {"approver_id",      r.val(i,"approver_id").empty() ? json(nullptr) : json(r.val(i,"approver_id"))},
        {"estado",           r.val(i,"estado")},
        {"comentarios",      r.val(i,"comentarios").empty() ? json(nullptr) : json(r.val(i,"comentarios"))},
        {"fecha_resolucion", r.val(i,"fecha_resolucion").empty() ? json(nullptr) : json(r.val(i,"fecha_resolucion"))}
    };
}

// ── POST /api/v1/commercial/projects ─────────────────────────────────────────
void ProjectController::createProject(const HttpRequestPtr& req,
                                       std::function<void(const HttpResponsePtr&)>&& cb) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    json body;
    try { body = json::parse(req->getBody()); }
    catch (...) { cb(jsonResp(k400BadRequest, {{"error","JSON mal formado"}})); return; }

    if (!body.contains("client_id") || !body["client_id"].is_string())
        { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: client_id"}})); return; }
    if (!body.contains("nombre") || body["nombre"].get<std::string>().empty())
        { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: nombre"}})); return; }

    try {
        auto project = Project::create(claims->tenant_id, claims->user_id, body);
        cb(jsonResp(k201Created, {{"project", project.toJson()}}));
    } catch (const std::exception& e) {
        std::string msg = e.what();
        if (msg.find("violates foreign key") != std::string::npos)
            cb(jsonResp(k400BadRequest, {{"error","client_id inválido"}}));
        else
            cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// ── GET /api/v1/commercial/projects ──────────────────────────────────────────
void ProjectController::listProjects(const HttpRequestPtr& req,
                                      std::function<void(const HttpResponsePtr&)>&& cb) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    std::string client_id = req->getParameter("client_id");

    try {
        auto projects = Project::findAll(claims->tenant_id, client_id);
        json arr = json::array();
        for (const auto& p : projects) arr.push_back(p.toJson());
        cb(jsonResp(k200OK, {{"projects", arr}, {"total", arr.size()}}));
    } catch (const std::exception&) {
        cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// ── GET /api/v1/commercial/projects/{id} ─────────────────────────────────────
void ProjectController::getProject(const HttpRequestPtr& req,
                                    std::function<void(const HttpResponsePtr&)>&& cb,
                                    std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        auto project = Project::findById(id, claims->tenant_id);
        if (!project) { cb(jsonResp(k404NotFound, {{"error","Proyecto no encontrado"}})); return; }

        json body = project->toJson();

        // Aprobaciones
        auto appr = PgConnection::getInstance().exec(
            "SELECT * FROM public.project_approvals WHERE project_id=$1 ORDER BY created_at", {id});
        json apprArr = json::array();
        for (int i = 0; i < appr.rows(); ++i) apprArr.push_back(approvalRowToJson(appr, i));
        body["approvals"] = apprArr;

        // Solicitudes de producto
        auto prs = ProductRequest::findByProject(id);
        json prArr = json::array();
        for (const auto& pr : prs) prArr.push_back(pr.toJson());
        body["product_requests"] = prArr;

        // Equipo del proyecto
        auto members = PgConnection::getInstance().exec(
            "SELECT pm.id, pm.rol_proyecto, pm.assigned_at, pm.activo, "
            "       e.id AS emp_id, e.nombres, e.apellidos, e.cargo, e.departamento, "
            "       e.email_corporativo, e.telefono1 "
            "FROM public.project_members pm "
            "JOIN public.employees e ON e.id = pm.employee_id "
            "WHERE pm.project_id=$1 ORDER BY pm.rol_proyecto, e.apellidos", {id});
        json membersArr = json::array();
        for (int i = 0; i < members.rows(); ++i) {
            membersArr.push_back({
                {"id",               members.val(i,"id")},
                {"rol_proyecto",     members.val(i,"rol_proyecto")},
                {"activo",           members.val(i,"activo") == "t"},
                {"assigned_at",      members.val(i,"assigned_at")},
                {"employee", {
                    {"id",                members.val(i,"emp_id")},
                    {"nombre_completo",   members.val(i,"nombres") + " " + members.val(i,"apellidos")},
                    {"cargo",             members.val(i,"cargo")},
                    {"departamento",      members.val(i,"departamento")},
                    {"email_corporativo", members.val(i,"email_corporativo")},
                    {"telefono1",         members.val(i,"telefono1")}
                }}
            });
        }
        body["members"] = membersArr;

        cb(jsonResp(k200OK, {{"project", body}}));
    } catch (const std::exception&) {
        cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// ── POST /api/v1/commercial/projects/{id}/submit ─────────────────────────────
void ProjectController::submitProject(const HttpRequestPtr& req,
                                       std::function<void(const HttpResponsePtr&)>&& cb,
                                       std::string&& id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        Project::submitForApproval(id, claims->tenant_id);
        cb(jsonResp(k200OK, {{"message","Proyecto enviado a revisión. Las áreas fueron notificadas."}}));
    } catch (const std::exception& e) {
        std::string msg = e.what();
        if (msg.find("borrador") != std::string::npos)
            cb(jsonResp(k409Conflict, {{"error", msg}}));
        else if (msg.find("no encontrado") != std::string::npos)
            cb(jsonResp(k404NotFound, {{"error", msg}}));
        else
            cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// ── POST /api/v1/commercial/projects/{id}/product-requests ───────────────────
void ProjectController::createProductReq(const HttpRequestPtr& req,
                                          std::function<void(const HttpResponsePtr&)>&& cb,
                                          std::string&& project_id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    json body;
    try { body = json::parse(req->getBody()); }
    catch (...) { cb(jsonResp(k400BadRequest, {{"error","JSON mal formado"}})); return; }

    if (!body.contains("nombre_producto") || body["nombre_producto"].get<std::string>().empty())
        { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: nombre_producto"}})); return; }
    if (!body.contains("descripcion_necesidad") || body["descripcion_necesidad"].get<std::string>().empty())
        { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: descripcion_necesidad"}})); return; }
    if (!body.contains("cantidad_estimada") || !body["cantidad_estimada"].is_number_integer()
        || body["cantidad_estimada"].get<int>() < 1)
        { cb(jsonResp(k400BadRequest, {{"error","cantidad_estimada debe ser un entero mayor a 0"}})); return; }

    try {
        auto pr = ProductRequest::create(project_id, claims->tenant_id, claims->user_id, body);
        cb(jsonResp(k201Created, {{"product_request", pr.toJson()}}));
    } catch (const std::exception& e) {
        std::string msg = e.what();
        if (msg.find("violates foreign key") != std::string::npos)
            cb(jsonResp(k400BadRequest, {{"error","project_id inválido"}}));
        else
            cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// ── GET /api/v1/commercial/projects/{id}/product-requests ────────────────────
void ProjectController::listProductReqs(const HttpRequestPtr& req,
                                         std::function<void(const HttpResponsePtr&)>&& cb,
                                         std::string&& project_id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        auto prs = ProductRequest::findByProject(project_id);
        json arr = json::array();
        for (const auto& pr : prs) arr.push_back(pr.toJson());
        cb(jsonResp(k200OK, {{"product_requests", arr}, {"total", arr.size()}}));
    } catch (const std::exception&) {
        cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// ── PATCH /api/v1/commercial/projects/{id}/approvals/{dept} ──────────────────
void ProjectController::resolveApproval(const HttpRequestPtr& req,
                                         std::function<void(const HttpResponsePtr&)>&& cb,
                                         std::string&& project_id,
                                         std::string&& dept) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    json body;
    try { body = json::parse(req->getBody()); }
    catch (...) { cb(jsonResp(k400BadRequest, {{"error","JSON mal formado"}})); return; }

    if (!body.contains("estado") || !body["estado"].is_string())
        { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: estado (aprobado|rechazado)"}})); return; }

    std::string estado = body["estado"].get<std::string>();
    if (estado != "aprobado" && estado != "rechazado")
        { cb(jsonResp(k400BadRequest, {{"error","estado debe ser 'aprobado' o 'rechazado'"}})); return; }

    std::string comentarios = body.contains("comentarios") && body["comentarios"].is_string()
        ? body["comentarios"].get<std::string>() : "";

    try {
        // Actualizar registro de aprobación
        auto r = PgConnection::getInstance().exec(
            "UPDATE public.project_approvals "
            "SET estado=$1, approver_id=$2, comentarios=NULLIF($3,''), fecha_resolucion=NOW() "
            "WHERE project_id=$4 AND departamento=$5 RETURNING *",
            {estado, claims->user_id, comentarios, project_id, dept}
        );

        if (!r.ok() || r.rows() == 0)
            { cb(jsonResp(k404NotFound, {{"error","Aprobación no encontrada o departamento inválido"}})); return; }

        if (estado == "rechazado") {
            // Devolver proyecto a borrador
            PgConnection::getInstance().exec(
                "UPDATE public.projects SET estado='borrador' WHERE id=$1", {project_id});
            cb(jsonResp(k200OK, {{"message","Proyecto rechazado. Vuelve a 'borrador' para correcciones."}}));
            return;
        }

        // Verificar si todos aprobaron
        auto count = PgConnection::getInstance().exec(
            "SELECT COUNT(*) FROM public.project_approvals WHERE project_id=$1 AND estado='aprobado'",
            {project_id}
        );
        int approved = count.ok() && count.rows() > 0 ? std::stoi(count.val(0,0)) : 0;

        if (approved == 4) {
            PgConnection::getInstance().exec(
                "UPDATE public.projects SET estado='aprobado' WHERE id=$1", {project_id});
            cb(jsonResp(k200OK, {{"message","Todas las áreas aprobaron. Proyecto listo para producción."}}));
        } else {
            cb(jsonResp(k200OK, {
                {"message","Aprobación registrada"},
                {"aprobaciones_pendientes", 4 - approved}
            }));
        }

    } catch (const std::exception&) {
        cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// ── POST /api/v1/commercial/projects/{id}/members ─────────────────────────────
void ProjectController::addMember(const HttpRequestPtr& req,
                                   std::function<void(const HttpResponsePtr&)>&& cb,
                                   std::string&& project_id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    json body;
    try { body = json::parse(req->getBody()); }
    catch (...) { cb(jsonResp(k400BadRequest, {{"error","JSON mal formado"}})); return; }

    if (!body.contains("employee_id") || !body["employee_id"].is_string())
        { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: employee_id"}})); return; }
    if (!body.contains("rol_proyecto") || !body["rol_proyecto"].is_string())
        { cb(jsonResp(k400BadRequest, {{"error","Campo requerido: rol_proyecto"}})); return; }

    static const std::set<std::string> VALID_ROLES = {
        "lider_proyectos","desarrollador","coordinador_produccion","observador"
    };
    if (!VALID_ROLES.count(body["rol_proyecto"].get<std::string>()))
        { cb(jsonResp(k400BadRequest, {{"error","rol_proyecto inválido"}})); return; }

    try {
        auto r = PgConnection::getInstance().exec(
            "INSERT INTO public.project_members (project_id, employee_id, rol_proyecto, assigned_by) "
            "VALUES ($1,$2,$3,$4) "
            "ON CONFLICT (project_id, employee_id, rol_proyecto) DO UPDATE SET activo=TRUE "
            "RETURNING id, rol_proyecto, assigned_at",
            {project_id,
             body["employee_id"].get<std::string>(),
             body["rol_proyecto"].get<std::string>(),
             claims->user_id}
        );
        if (!r.ok() || r.rows() == 0)
            throw std::runtime_error("insert failed");

        cb(jsonResp(k201Created, {
            {"message", "Miembro asignado al proyecto"},
            {"member_id", r.val(0,"id")},
            {"rol_proyecto", r.val(0,"rol_proyecto")}
        }));
    } catch (const std::exception& e) {
        std::string msg = e.what();
        if (msg.find("violates foreign key") != std::string::npos)
            cb(jsonResp(k400BadRequest, {{"error","employee_id o project_id inválido"}}));
        else
            cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// ── GET /api/v1/commercial/projects/{id}/members ──────────────────────────────
void ProjectController::listMembers(const HttpRequestPtr& req,
                                     std::function<void(const HttpResponsePtr&)>&& cb,
                                     std::string&& project_id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        auto r = PgConnection::getInstance().exec(
            "SELECT pm.id, pm.rol_proyecto, pm.activo, pm.assigned_at, "
            "       e.id AS emp_id, e.nombres, e.apellidos, e.cargo, "
            "       e.departamento, e.email_corporativo, e.telefono1 "
            "FROM public.project_members pm "
            "JOIN public.employees e ON e.id = pm.employee_id "
            "WHERE pm.project_id=$1 ORDER BY pm.rol_proyecto, e.apellidos",
            {project_id}
        );
        json arr = json::array();
        for (int i = 0; i < r.rows(); ++i) {
            arr.push_back({
                {"id",           r.val(i,"id")},
                {"rol_proyecto", r.val(i,"rol_proyecto")},
                {"activo",       r.val(i,"activo") == "t"},
                {"assigned_at",  r.val(i,"assigned_at")},
                {"employee", {
                    {"id",                r.val(i,"emp_id")},
                    {"nombre_completo",   r.val(i,"nombres") + " " + r.val(i,"apellidos")},
                    {"cargo",             r.val(i,"cargo")},
                    {"departamento",      r.val(i,"departamento")},
                    {"email_corporativo", r.val(i,"email_corporativo")},
                    {"telefono1",         r.val(i,"telefono1")}
                }}
            });
        }
        cb(jsonResp(k200OK, {{"members", arr}, {"total", arr.size()}}));
    } catch (const std::exception&) {
        cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}

// ── DELETE /api/v1/commercial/projects/{id}/members/{member_id} ───────────────
void ProjectController::removeMember(const HttpRequestPtr& req,
                                      std::function<void(const HttpResponsePtr&)>&& cb,
                                      std::string&& project_id,
                                      std::string&& member_id) {
    auto claims = JwtUtils::extractClaims(req);
    if (!claims) { cb(JwtUtils::unauthorized()); return; }

    try {
        auto r = PgConnection::getInstance().exec(
            "UPDATE public.project_members SET activo=FALSE "
            "WHERE id=$1 AND project_id=$2 RETURNING id",
            {member_id, project_id}
        );
        if (!r.ok() || r.rows() == 0)
            { cb(jsonResp(k404NotFound, {{"error","Miembro no encontrado"}})); return; }

        cb(jsonResp(k200OK, {{"message","Miembro removido del proyecto"}}));
    } catch (const std::exception&) {
        cb(jsonResp(k500InternalServerError, {{"error","Error interno del servidor"}}));
    }
}
