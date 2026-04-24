#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "../../utils/PgConnection.h"

using json = nlohmann::json;

struct EmployeeEmergencyContact {
    std::string id;
    std::string employee_id;
    std::string tenant_id;
    std::string nombres;
    std::string apellidos;
    std::string parentesco;
    std::string telefono1;
    std::string telefono2;
    std::string email;
    bool        es_principal{false};
    std::string created_at;
    std::string updated_at;

    json toJson() const;

    static EmployeeEmergencyContact                create(const std::string& employee_id,
                                                           const std::string& tenant_id,
                                                           const json& data);
    static std::vector<EmployeeEmergencyContact>   findByEmployee(const std::string& employee_id,
                                                                   const std::string& tenant_id);
    static bool                                    remove(const std::string& id,
                                                          const std::string& employee_id,
                                                          const std::string& tenant_id);
};
