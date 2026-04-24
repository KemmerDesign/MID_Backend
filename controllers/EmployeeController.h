#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class EmployeeController : public drogon::HttpController<EmployeeController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(EmployeeController::createEmployee, "/api/v1/rrhh/employees",     Post);
    ADD_METHOD_TO(EmployeeController::listEmployees,  "/api/v1/rrhh/employees",     Get);
    ADD_METHOD_TO(EmployeeController::getEmployee,    "/api/v1/rrhh/employees/{1}", Get);
    METHOD_LIST_END

    void createEmployee(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void listEmployees (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void getEmployee   (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::string&& id);
};
