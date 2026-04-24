#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class EmployeeController : public drogon::HttpController<EmployeeController> {
public:
    METHOD_LIST_BEGIN
    // ── CRUD base ─────────────────────────────────────────────────────────────
    ADD_METHOD_TO(EmployeeController::createEmployee, "/api/v1/rrhh/employees",     Post);
    ADD_METHOD_TO(EmployeeController::listEmployees,  "/api/v1/rrhh/employees",     Get);
    ADD_METHOD_TO(EmployeeController::getEmployee,    "/api/v1/rrhh/employees/{1}", Get);
    // ── Historial laboral ─────────────────────────────────────────────────────
    ADD_METHOD_TO(EmployeeController::applyPositionChange, "/api/v1/rrhh/employees/{1}/position-change", Post);
    ADD_METHOD_TO(EmployeeController::listPositions,       "/api/v1/rrhh/employees/{1}/positions",       Get);
    // ── Dependientes (hijos, cónyuge) ─────────────────────────────────────────
    ADD_METHOD_TO(EmployeeController::addDependent,    "/api/v1/rrhh/employees/{1}/dependents",      Post);
    ADD_METHOD_TO(EmployeeController::listDependents,  "/api/v1/rrhh/employees/{1}/dependents",      Get);
    ADD_METHOD_TO(EmployeeController::removeDependent, "/api/v1/rrhh/employees/{1}/dependents/{2}",  Delete);
    // ── Ausencias (incapacidades, licencias, permisos) ────────────────────────
    ADD_METHOD_TO(EmployeeController::addAbsence,       "/api/v1/rrhh/employees/{1}/absences",      Post);
    ADD_METHOD_TO(EmployeeController::listAbsences,     "/api/v1/rrhh/employees/{1}/absences",      Get);
    ADD_METHOD_TO(EmployeeController::updateAbsence,    "/api/v1/rrhh/employees/{1}/absences/{2}",  Patch);
    // ── Vacaciones ────────────────────────────────────────────────────────────
    ADD_METHOD_TO(EmployeeController::ensureVacationPeriod, "/api/v1/rrhh/employees/{1}/vacations",      Post);
    ADD_METHOD_TO(EmployeeController::listVacations,        "/api/v1/rrhh/employees/{1}/vacations",      Get);
    ADD_METHOD_TO(EmployeeController::updateVacation,       "/api/v1/rrhh/employees/{1}/vacations/{2}",  Patch);
    ADD_METHOD_TO(EmployeeController::removeVacation,       "/api/v1/rrhh/employees/{1}/vacations/{2}",  Delete);
    // ── Disciplinario ─────────────────────────────────────────────────────────
    ADD_METHOD_TO(EmployeeController::addDisciplinary,    "/api/v1/rrhh/employees/{1}/disciplinary",      Post);
    ADD_METHOD_TO(EmployeeController::listDisciplinary,   "/api/v1/rrhh/employees/{1}/disciplinary",      Get);
    ADD_METHOD_TO(EmployeeController::updateDisciplinary, "/api/v1/rrhh/employees/{1}/disciplinary/{2}",  Patch);
    // ── Contactos de emergencia ───────────────────────────────────────────────
    ADD_METHOD_TO(EmployeeController::addEmergencyContact,    "/api/v1/rrhh/employees/{1}/emergency-contacts",      Post);
    ADD_METHOD_TO(EmployeeController::listEmergencyContacts,  "/api/v1/rrhh/employees/{1}/emergency-contacts",      Get);
    ADD_METHOD_TO(EmployeeController::removeEmergencyContact, "/api/v1/rrhh/employees/{1}/emergency-contacts/{2}",  Delete);
    // ── Documentos ───────────────────────────────────────────────────────────
    ADD_METHOD_TO(EmployeeController::addDocument,    "/api/v1/rrhh/employees/{1}/documents",      Post);
    ADD_METHOD_TO(EmployeeController::listDocuments,  "/api/v1/rrhh/employees/{1}/documents",      Get);
    ADD_METHOD_TO(EmployeeController::updateDocument, "/api/v1/rrhh/employees/{1}/documents/{2}",  Patch);
    ADD_METHOD_TO(EmployeeController::removeDocument, "/api/v1/rrhh/employees/{1}/documents/{2}",  Delete);
    // ── Capacitaciones y certificaciones ─────────────────────────────────────
    ADD_METHOD_TO(EmployeeController::addTraining,    "/api/v1/rrhh/employees/{1}/training",      Post);
    ADD_METHOD_TO(EmployeeController::listTraining,   "/api/v1/rrhh/employees/{1}/training",      Get);
    ADD_METHOD_TO(EmployeeController::updateTraining, "/api/v1/rrhh/employees/{1}/training/{2}",  Patch);
    ADD_METHOD_TO(EmployeeController::removeTraining, "/api/v1/rrhh/employees/{1}/training/{2}",  Delete);
    // ── Educación formal ─────────────────────────────────────────────────────
    ADD_METHOD_TO(EmployeeController::addEducation,    "/api/v1/rrhh/employees/{1}/education",      Post);
    ADD_METHOD_TO(EmployeeController::listEducation,   "/api/v1/rrhh/employees/{1}/education",      Get);
    ADD_METHOD_TO(EmployeeController::removeEducation, "/api/v1/rrhh/employees/{1}/education/{2}",  Delete);
    // ── Experiencia laboral previa ────────────────────────────────────────────
    ADD_METHOD_TO(EmployeeController::addWorkHistory,    "/api/v1/rrhh/employees/{1}/work-history",      Post);
    ADD_METHOD_TO(EmployeeController::listWorkHistory,   "/api/v1/rrhh/employees/{1}/work-history",      Get);
    ADD_METHOD_TO(EmployeeController::removeWorkHistory, "/api/v1/rrhh/employees/{1}/work-history/{2}",  Delete);
    METHOD_LIST_END

    void createEmployee(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void listEmployees (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void getEmployee   (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);

    void applyPositionChange(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void listPositions      (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);

    void addDependent   (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void listDependents (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void removeDependent(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id, std::string&& dep_id);

    void addAbsence   (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void listAbsences (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void updateAbsence(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id, std::string&& abs_id);

    void ensureVacationPeriod(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void listVacations       (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void updateVacation      (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id, std::string&& vac_id);

    void addDisciplinary   (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void listDisciplinary  (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void updateDisciplinary(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id, std::string&& disc_id);

    void addEmergencyContact   (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void listEmergencyContacts (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void removeEmergencyContact(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id, std::string&& contact_id);

    void addDocument   (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void listDocuments (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void updateDocument(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id, std::string&& doc_id);
    void removeDocument(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id, std::string&& doc_id);

    void addTraining   (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void listTraining  (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void updateTraining(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id, std::string&& training_id);
    void removeTraining(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id, std::string&& training_id);

    void addEducation   (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void listEducation  (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void removeEducation(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id, std::string&& edu_id);

    void addWorkHistory   (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void listWorkHistory  (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void removeWorkHistory(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id, std::string&& history_id);
    void removeVacation   (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id, std::string&& vac_id);
};
