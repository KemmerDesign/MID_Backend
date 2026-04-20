#include "Database.h"
#include <fmt/core.h>

// ─── Por qué "monarca.db"? ────────────────────────────────────────────────────
// SQLite guarda TODO en un solo archivo. Sin servidor, sin credenciales,
// sin internet. El archivo se crea solo si no existe.
// ─────────────────────────────────────────────────────────────────────────────
static constexpr const char* DB_FILE = "monarca.db";

// ─── El Singleton ─────────────────────────────────────────────────────────────
// La variable 'instance' es static LOCAL — se construye la primera vez que
// alguien llama getInstance() y vive hasta que el programa termina.
// Desde C++11 esto es thread-safe garantizado por el estándar.
// ─────────────────────────────────────────────────────────────────────────────
Database& Database::getInstance() {
    static Database instance;
    return instance;
}

// ─── Constructor: aquí abre el archivo SQLite (RAII) ─────────────────────────
// SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE:
//   - Si el archivo existe → lo abre
//   - Si no existe → lo crea
// Si falla (disco lleno, permisos), SQLiteCpp lanza una excepción aquí,
// antes de que la app haga cualquier cosa. Fallo rápido y claro.
// ─────────────────────────────────────────────────────────────────────────────
Database::Database()
    : db_(DB_FILE, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
    fmt::print("📂 Base de datos abierta: {}\n", DB_FILE);
    initSchema();
}

// ─── initSchema: crea la tabla si no existe ──────────────────────────────────
// IF NOT EXISTS: esta línea puede ejecutarse 1000 veces sin romper nada.
// Guardamos el JSON como texto (TEXT) — SQLite no tiene tipo JSON nativo,
// pero nlohmann nos serializa/deserializa sin esfuerzo.
// PRIMARY KEY (collection, doc_id): un documento es único por su par
// colección + id, igual que en Firestore.
// ─────────────────────────────────────────────────────────────────────────────
void Database::initSchema() {
    db_.exec(R"(
        CREATE TABLE IF NOT EXISTS documents (
            collection  TEXT NOT NULL,
            doc_id      TEXT NOT NULL,
            data        TEXT NOT NULL,
            PRIMARY KEY (collection, doc_id)
        )
    )");
}

// ─── save: INSERT OR REPLACE ──────────────────────────────────────────────────
// INSERT OR REPLACE es el "upsert" de SQLite: si el par (collection, doc_id)
// ya existe lo reemplaza, si no existe lo crea. Mismo comportamiento que
// el PATCH que hacía FirebaseClient con Firestore.
//
// Statement con '?' (parámetros ligados): NUNCA concatenes strings para SQL.
// Concatenar es la causa #1 de SQL injection. SQLiteCpp con bind() escapa
// automáticamente los valores.
// ─────────────────────────────────────────────────────────────────────────────
bool Database::save(const std::string& collection, const std::string& docId, const json& data) {
    try {
        SQLite::Statement stmt(db_,
            "INSERT OR REPLACE INTO documents (collection, doc_id, data) VALUES (?, ?, ?)");

        stmt.bind(1, collection);
        stmt.bind(2, docId);
        stmt.bind(3, data.dump()); // json → string

        stmt.exec();
        fmt::print("✅ Guardado: {}/{}\n", collection, docId);
        return true;

    } catch (const SQLite::Exception& e) {
        fmt::print(stderr, "❌ Error DB save ({}/{}): {}\n", collection, docId, e.what());
        return false;
    }
}

// ─── get: SELECT + std::optional ─────────────────────────────────────────────
// executeStep() avanza al primer resultado. Si devuelve false, no hay fila.
// Devolvemos std::nullopt (vacío) en vez de nullptr o json inválido —
// el llamador puede hacer: if (auto doc = db.get(...)) { usar *doc }
// ─────────────────────────────────────────────────────────────────────────────
std::optional<json> Database::get(const std::string& collection, const std::string& docId) {
    try {
        SQLite::Statement stmt(db_,
            "SELECT data FROM documents WHERE collection = ? AND doc_id = ?");

        stmt.bind(1, collection);
        stmt.bind(2, docId);

        if (stmt.executeStep()) {
            std::string raw = stmt.getColumn(0).getText();
            return json::parse(raw); // string → json
        }

        return std::nullopt; // No encontrado

    } catch (const SQLite::Exception& e) {
        fmt::print(stderr, "❌ Error DB get ({}/{}): {}\n", collection, docId, e.what());
        return std::nullopt;
    }
}
