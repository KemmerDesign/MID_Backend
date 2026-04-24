#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "../../utils/PgConnection.h"

using json = nlohmann::json;

struct Client {
    std::string id;
    std::string tenant_id;
    std::string nombre;
    std::string nombre_comercial;
    std::string nit;
    std::string direccion_principal;
    std::string ciudad_principal;
    std::string correo_facturacion;
    std::string sitio_web;
    std::string sector;
    bool        activo = true;
    std::string notas;
    std::string created_at;
    std::string updated_at;

    json toJson() const;

    static Client                create(const std::string& tenant_id, const json& data);
    static std::optional<Client> findById(const std::string& id, const std::string& tenant_id);
    static std::vector<Client>   findAll(const std::string& tenant_id);
};
