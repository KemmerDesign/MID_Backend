#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "../../utils/PgConnection.h"

using json = nlohmann::json;

struct ProductRequest {
    std::string id;
    std::string project_id;
    std::string tenant_id;
    std::string commercial_id;
    std::string nombre_producto;
    std::string descripcion_necesidad;
    int         cantidad_estimada = 0;
    std::string presupuesto_estimado;
    std::string unidad_medida;
    std::string estado;
    std::string notas_adicionales;
    std::string created_at;
    std::string updated_at;

    json toJson() const;

    static ProductRequest              create(const std::string& project_id,
                                              const std::string& tenant_id,
                                              const std::string& commercial_id,
                                              const json& data);
    static std::vector<ProductRequest> findByProject(const std::string& project_id);
};
