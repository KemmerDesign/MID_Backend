#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "../../utils/PgConnection.h"

using json = nlohmann::json;

struct EmployeeDependent {
    std::string id;
    std::string employee_id;
    std::string tenant_id;
    std::string nombres;
    std::string apellidos;
    std::string fecha_nacimiento;
    std::string parentesco;
    bool        vive_con_empleado{true};
    bool        activo{true};
    std::string notas;
    std::string created_at;
    std::string updated_at;

    json toJson() const;

    static EmployeeDependent                create(const std::string& employee_id,
                                                    const std::string& tenant_id,
                                                    const json& data);
    static std::vector<EmployeeDependent>   findByEmployee(const std::string& employee_id,
                                                            const std::string& tenant_id,
                                                            bool only_active = true);
    static bool                             deactivate(const std::string& id,
                                                       const std::string& employee_id,
                                                       const std::string& tenant_id);
};
