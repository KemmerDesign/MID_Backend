#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "../../utils/PgConnection.h"

using json = nlohmann::json;

struct ProductCatalog {
    std::string id;
    std::string tenant_id;
    std::string nombre;
    std::string categoria;
    std::string descripcion;
    bool        activo{true};
    std::string created_at;
    std::string updated_at;

    json toJson() const;

    static ProductCatalog                create(const std::string& tenant_id, const json& data);
    static std::vector<ProductCatalog>   findAll(const std::string& tenant_id, bool only_active = true);
    static std::optional<ProductCatalog> findById(const std::string& id, const std::string& tenant_id);
    static std::optional<ProductCatalog> update(const std::string& id,
                                                const std::string& tenant_id,
                                                const json& data);
};
