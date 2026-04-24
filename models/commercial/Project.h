#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "../../utils/PgConnection.h"

using json = nlohmann::json;

struct Project {
    std::string id;
    std::string tenant_id;
    std::string client_id;
    std::string commercial_id;
    std::string nombre;
    std::string descripcion;
    std::string estado;
    std::string fecha_prometida;
    std::string presupuesto_cliente;
    std::string notas;
    std::string created_at;
    std::string updated_at;

    json toJson() const;

    static Project                create(const std::string& tenant_id,
                                         const std::string& commercial_id, const json& data);
    static std::optional<Project> findById(const std::string& id, const std::string& tenant_id);
    static std::vector<Project>   findAll(const std::string& tenant_id,
                                          const std::string& client_id = "");
    static void submitForApproval(const std::string& project_id, const std::string& tenant_id);
};
