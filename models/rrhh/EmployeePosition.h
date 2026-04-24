#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "../../utils/PgConnection.h"

using json = nlohmann::json;

struct EmployeePosition {
    std::string id;
    std::string employee_id;
    std::string tenant_id;
    std::string rol_id;
    std::string cargo;
    std::string departamento;
    std::string salario_base;
    std::string fecha_inicio;
    std::string fecha_fin;
    std::string motivo;
    std::string notas;
    std::string registrado_por;
    bool        es_honorarios{false};
    std::string created_at;

    json toJson() const;

    // Crea un nuevo período (cierra el activo anterior automáticamente)
    // y actualiza el snapshot en employees.
    static EmployeePosition applyChange(const std::string& employee_id,
                                        const std::string& tenant_id,
                                        const std::string& registrado_por,
                                        const json& data);

    static std::vector<EmployeePosition> findByEmployee(const std::string& employee_id,
                                                         const std::string& tenant_id);
};
