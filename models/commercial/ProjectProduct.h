#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "../../utils/PgConnection.h"

using json = nlohmann::json;

struct MaterialSuggestion {
    std::string id;
    std::string tipo_material;
    std::string descripcion;
    std::string sugerido_por;
    std::string created_at;

    json toJson() const;
};

struct ProjectProduct {
    std::string id;
    std::string project_id;
    std::string tenant_id;
    std::string catalog_id;
    std::string nombre;
    std::string categoria;
    std::string alto_mm;
    std::string ancho_mm;
    std::string profundidad_mm;
    std::string canal_venta;
    std::string temporalidad;
    std::string temporalidad_meses;
    std::string cantidad;
    std::string presupuesto_estimado;
    std::string notas;
    std::string created_at;
    std::string updated_at;

    std::vector<MaterialSuggestion> materiales;

    json toJson() const;

    static ProjectProduct                create(const std::string& project_id,
                                                const std::string& tenant_id,
                                                const json& data);
    static std::vector<ProjectProduct>   findByProject(const std::string& project_id,
                                                        const std::string& tenant_id);
    static std::optional<ProjectProduct> findById(const std::string& id,
                                                   const std::string& tenant_id);
    static bool                          remove(const std::string& id,
                                                const std::string& tenant_id);

    static MaterialSuggestion addMaterial(const std::string& project_product_id,
                                          const std::string& tenant_id,
                                          const json& data,
                                          const std::string& sugerido_por = "");
    static bool                removeMaterial(const std::string& material_id,
                                              const std::string& project_product_id,
                                              const std::string& tenant_id);
};
