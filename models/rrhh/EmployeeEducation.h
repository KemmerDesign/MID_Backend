#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "../../utils/PgConnection.h"

using json = nlohmann::json;

struct EmployeeEducation {
    std::string id;
    std::string employee_id;
    std::string tenant_id;
    std::string nivel;
    std::string titulo;
    std::string institucion;
    std::string anio_inicio;
    std::string anio_graduacion;
    bool        en_curso{false};
    std::string ciudad;
    std::string notas;
    std::string created_at;
    std::string updated_at;

    json toJson() const;

    static EmployeeEducation                create(const std::string& employee_id,
                                                    const std::string& tenant_id,
                                                    const json& data);
    static std::vector<EmployeeEducation>   findByEmployee(const std::string& employee_id,
                                                            const std::string& tenant_id);
    static bool                             remove(const std::string& id,
                                                   const std::string& employee_id,
                                                   const std::string& tenant_id);
};
