#pragma once
#include <string>
#include <optional>
#include <nlohmann/json.hpp>
#include <SQLiteCpp/SQLiteCpp.h>

// ─── Por qué std::optional? ──────────────────────────────────────────────────
// get() puede no encontrar nada — en vez de devolver nullptr (peligroso con
// JSON) o lanzar una excepción, devolvemos optional<json>: el llamador sabe
// explícitamente si hubo resultado o no. Es C++ moderno idiomático.
// ─────────────────────────────────────────────────────────────────────────────

using json = nlohmann::json;

class Database {
public:
    // Singleton: una sola conexión a la BD en toda la app
    static Database& getInstance();

    // Regla de tres/cinco: si el Singleton no se copia, lo prohibimos
    Database(const Database&)            = delete;
    Database& operator=(const Database&) = delete;

    // Guarda o actualiza un documento. Devuelve true si tuvo éxito.
    bool save(const std::string& collection, const std::string& docId, const json& data);

    // Busca un documento. Devuelve nullopt si no existe o hay error.
    std::optional<json> get(const std::string& collection, const std::string& docId);

private:
    // ─── RAII en acción ──────────────────────────────────────────────────────
    // SQLite::Database es un objeto que abre el archivo en su constructor
    // y lo cierra en su destructor — nosotros no hacemos nada manual.
    // Cuando Database (Singleton) se destruye al cerrar la app, db_ se
    // destruye también y la conexión queda limpia. Eso es RAII.
    // ─────────────────────────────────────────────────────────────────────────
    SQLite::Database db_;

    Database(); // Constructor privado: solo getInstance() puede crear la instancia
    void initSchema(); // Crea la tabla si no existe
};
