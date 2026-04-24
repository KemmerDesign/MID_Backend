#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "../../utils/PgConnection.h"

using json = nlohmann::json;

struct EmployeeTraining {
    std::string id;
    std::string employee_id;
    std::string tenant_id;
    std::string nombre_curso;
    std::string institucion;
    std::string tipo;
    std::string modalidad;
    std::string fecha_inicio;
    std::string fecha_fin;
    std::string horas;
    std::string resultado;
    std::string fecha_vencimiento;
    std::string certificado_url;
    std::string notas;
    std::string created_at;
    std::string updated_at;

    json toJson() const;

    static EmployeeTraining                create(const std::string& employee_id,
                                                   const std::string& tenant_id,
                                                   const json& data);
    static std::vector<EmployeeTraining>   findByEmployee(const std::string& employee_id,
                                                           const std::string& tenant_id);
    static std::optional<EmployeeTraining> update(const std::string& id,
                                                   const std::string& employee_id,
                                                   const std::string& tenant_id,
                                                   const json& data);
    static bool                            remove(const std::string& id,
                                                  const std::string& employee_id,
                                                  const std::string& tenant_id);
};
