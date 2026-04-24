#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "../../utils/PgConnection.h"

using json = nlohmann::json;

struct EmployeeDocument {
    std::string id;
    std::string employee_id;
    std::string tenant_id;
    std::string tipo;
    std::string nombre;
    std::string url_referencia;
    std::string fecha_entrega;
    std::string fecha_vencimiento;
    bool        obligatorio{true};
    std::string estado;
    std::string notas;
    std::string created_at;
    std::string updated_at;

    json toJson() const;

    static EmployeeDocument                create(const std::string& employee_id,
                                                   const std::string& tenant_id,
                                                   const json& data);
    static std::vector<EmployeeDocument>   findByEmployee(const std::string& employee_id,
                                                           const std::string& tenant_id);
    static std::optional<EmployeeDocument> updateStatus(const std::string& id,
                                                         const std::string& employee_id,
                                                         const std::string& tenant_id,
                                                         const json& data);
    static bool                            remove(const std::string& id,
                                                  const std::string& employee_id,
                                                  const std::string& tenant_id);
};
