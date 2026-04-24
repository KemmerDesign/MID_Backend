#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "../../utils/PgConnection.h"

using json = nlohmann::json;

struct EmployeeVacation {
    std::string id;
    std::string employee_id;
    std::string tenant_id;
    int         periodo_year{0};
    std::string dias_disponibles;
    std::string dias_tomados;
    std::string fecha_inicio;
    std::string fecha_fin;
    std::string estado;
    std::string aprobado_por;
    std::string notas;
    std::string created_at;
    std::string updated_at;

    json toJson() const;

    // Crea el período si no existe, o devuelve el existente
    static EmployeeVacation     ensurePeriod(const std::string& employee_id,
                                              const std::string& tenant_id,
                                              int year,
                                              double dias_disponibles = 15.0);
    static std::vector<EmployeeVacation> findByEmployee(const std::string& employee_id,
                                                         const std::string& tenant_id);
    static std::optional<EmployeeVacation> update(const std::string& id,
                                                   const std::string& employee_id,
                                                   const std::string& tenant_id,
                                                   const json& data,
                                                   const std::string& aprobado_por = "");
    static bool remove(const std::string& id, const std::string& employee_id, const std::string& tenant_id);
};
