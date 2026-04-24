#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "../../utils/PgConnection.h"

using json = nlohmann::json;

struct EmployeeDisciplinary {
    std::string id;
    std::string employee_id;
    std::string tenant_id;
    std::string tipo;
    std::string fecha;
    std::string motivo;
    std::string descripcion;
    std::string estado;
    bool        firmado_por_empleado{false};
    std::string fecha_firma;
    std::string created_by;
    std::string created_at;
    std::string updated_at;

    json toJson() const;

    static EmployeeDisciplinary                create(const std::string& employee_id,
                                                       const std::string& tenant_id,
                                                       const std::string& created_by,
                                                       const json& data);
    static std::vector<EmployeeDisciplinary>   findByEmployee(const std::string& employee_id,
                                                               const std::string& tenant_id);
    static std::optional<EmployeeDisciplinary> update(const std::string& id,
                                                       const std::string& employee_id,
                                                       const std::string& tenant_id,
                                                       const json& data);
};
