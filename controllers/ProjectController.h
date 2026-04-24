#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class ProjectController : public drogon::HttpController<ProjectController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ProjectController::createProject,    "/api/v1/commercial/projects",                      Post);
    ADD_METHOD_TO(ProjectController::listProjects,     "/api/v1/commercial/projects",                      Get);
    ADD_METHOD_TO(ProjectController::getProject,       "/api/v1/commercial/projects/{1}",                  Get);
    ADD_METHOD_TO(ProjectController::submitProject,    "/api/v1/commercial/projects/{1}/submit",           Post);
    ADD_METHOD_TO(ProjectController::createProductReq, "/api/v1/commercial/projects/{1}/product-requests", Post);
    ADD_METHOD_TO(ProjectController::listProductReqs,  "/api/v1/commercial/projects/{1}/product-requests", Get);
    ADD_METHOD_TO(ProjectController::resolveApproval,  "/api/v1/commercial/projects/{1}/approvals/{2}",    Patch);
    ADD_METHOD_TO(ProjectController::addMember,        "/api/v1/commercial/projects/{1}/members",          Post);
    ADD_METHOD_TO(ProjectController::listMembers,      "/api/v1/commercial/projects/{1}/members",          Get);
    ADD_METHOD_TO(ProjectController::removeMember,     "/api/v1/commercial/projects/{1}/members/{2}",      Delete);
    METHOD_LIST_END

    void createProject   (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void listProjects    (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void getProject      (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void submitProject   (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
    void createProductReq(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& project_id);
    void listProductReqs (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& project_id);
    void resolveApproval (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& project_id, std::string&& dept);
    void addMember       (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& project_id);
    void listMembers     (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& project_id);
    void removeMember    (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& project_id, std::string&& member_id);
};
