#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "../../utils/PgConnection.h"

using json = nlohmann::json;

struct Role {
    std::string id;
    std::string tenant_id;
    std::string nombre;
    std::string descripcion;
    std::string departamento;
    int         nivel_jerarquico{1};
    bool        activo{true};
    std::string created_at;
    std::string updated_at;

    json toJson() const;

    static Role                create(const std::string& tenant_id, const json& data);
    static std::vector<Role>   findAll(const std::string& tenant_id,
                                       const std::string& departamento = "");
    static std::optional<Role> findById(const std::string& id, const std::string& tenant_id);
    static std::optional<Role> update(const std::string& id,
                                      const std::string& tenant_id,
                                      const json& data);
};
