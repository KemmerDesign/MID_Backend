#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "../../utils/PgConnection.h"

using json = nlohmann::json;

struct EmployeeAbsence {
    std::string id;
    std::string employee_id;
    std::string tenant_id;
    std::string tipo;
    std::string fecha_inicio;
    std::string fecha_fin;
    std::string dias_calendario;
    bool        remunerado{true};
    std::string descripcion;
    std::string documento_soporte;
    std::string estado;
    std::string aprobado_por;
    std::string created_by;
    bool        descuenta_domingo{false};
    std::string domingo_afectado;
    std::string created_at;
    std::string updated_at;

    json toJson() const;

    static EmployeeAbsence                create(const std::string& employee_id,
                                                  const std::string& tenant_id,
                                                  const std::string& created_by,
                                                  const json& data);
    static std::vector<EmployeeAbsence>   findByEmployee(const std::string& employee_id,
                                                          const std::string& tenant_id);
    static std::optional<EmployeeAbsence> updateStatus(const std::string& id,
                                                        const std::string& employee_id,
                                                        const std::string& tenant_id,
                                                        const std::string& estado,
                                                        const std::string& aprobado_por);
};
