#pragma once
#include <string>
#include <optional>
#include "../utils/PgConnection.h"

struct User {
    std::string id;
    std::string tenant_id;
    std::string email;
    std::string password_hash;
    std::string full_name;
    std::string role;
    bool        active = true;

    // Busca el usuario por email. Devuelve nullopt si no existe.
    static std::optional<User> findByEmail(const std::string& email);

    // Inserta un nuevo usuario. Devuelve el usuario creado (con id generado por PG).
    // Lanza std::runtime_error si el email ya existe o hay error de BD.
    static User create(const std::string& tenant_id,
                       const std::string& email,
                       const std::string& password_hash,
                       const std::string& full_name,
                       const std::string& role = "employee");
};
