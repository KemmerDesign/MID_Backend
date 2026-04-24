#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "../../utils/PgConnection.h"

using json = nlohmann::json;

struct EmployeeWorkHistory {
    std::string id;
    std::string employee_id;
    std::string tenant_id;
    std::string empresa;
    std::string cargo;
    std::string departamento;
    std::string fecha_inicio;
    std::string fecha_fin;
    std::string descripcion;
    std::string motivo_retiro;
    std::string created_at;
    std::string updated_at;

    json toJson() const;

    static EmployeeWorkHistory                create(const std::string& employee_id,
                                                      const std::string& tenant_id,
                                                      const json& data);
    static std::vector<EmployeeWorkHistory>   findByEmployee(const std::string& employee_id,
                                                              const std::string& tenant_id);
    static bool                               remove(const std::string& id,
                                                     const std::string& employee_id,
                                                     const std::string& tenant_id);
};
