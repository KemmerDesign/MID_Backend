#include "Database.h"
#include <fmt/core.h>

static constexpr const char* DB_FILE = "monarca.db";

Database& Database::getInstance() {
    static Database instance;
    return instance;
}

Database::Database()
    : db_(DB_FILE, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
    fmt::print("📂 Base de datos SQLite abierta: {}\n", DB_FILE);
    initSchema();
}

// Schema v2: tenant_id forma parte de la PK para aislamiento multi-tenant.
// La primera vez que corre en una DB con el schema v1 (sin tenant_id),
// migra los datos existentes y actualiza la versión.
void Database::initSchema() {
    db_.exec("CREATE TABLE IF NOT EXISTS __schema (key TEXT PRIMARY KEY, value TEXT)");

    int version = 0;
    try {
        SQLite::Statement q(db_, "SELECT value FROM __schema WHERE key='version'");
        if (q.executeStep()) version = std::stoi(q.getColumn(0).getText());
    } catch (...) {}

    if (version < 2) {
        // Migrar: si existe la tabla v1, copiar datos; si no, crear directo.
        db_.exec(R"(
            CREATE TABLE IF NOT EXISTS documents_v2 (
                tenant_id   TEXT NOT NULL DEFAULT '',
                collection  TEXT NOT NULL,
                doc_id      TEXT NOT NULL,
                data        TEXT NOT NULL,
                PRIMARY KEY (tenant_id, collection, doc_id)
            )
        )");
        // Intentar migrar datos del schema anterior (puede no existir)
        try {
            db_.exec("INSERT OR IGNORE INTO documents_v2 (tenant_id, collection, doc_id, data) "
                     "SELECT '', collection, doc_id, data FROM documents");
            db_.exec("DROP TABLE documents");
        } catch (...) {}
        // Si documents_v2 se renombra exitosamente (documents no existe ya)
        try { db_.exec("ALTER TABLE documents_v2 RENAME TO documents"); }
        catch (...) {}  // Si documents ya existe (ya migrado), ignorar

        db_.exec("INSERT OR REPLACE INTO __schema VALUES ('version', '2')");
        fmt::print("📂 SQLite: schema migrado a v2 (tenant_id en PK).\n");
    }
}

bool Database::save(const std::string& collection, const std::string& docId,
                     const json& data, const std::string& tenant_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    try {
        SQLite::Statement stmt(db_,
            "INSERT OR REPLACE INTO documents (tenant_id, collection, doc_id, data) "
            "VALUES (?, ?, ?, ?)");
        stmt.bind(1, tenant_id);
        stmt.bind(2, collection);
        stmt.bind(3, docId);
        stmt.bind(4, data.dump());
        stmt.exec();
        return true;
    } catch (const SQLite::Exception& e) {
        fmt::print(stderr, "❌ Error DB save ({}/{}/{}): {}\n", tenant_id, collection, docId, e.what());
        return false;
    }
}

std::optional<json> Database::get(const std::string& collection, const std::string& docId,
                                   const std::string& tenant_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    try {
        SQLite::Statement stmt(db_,
            "SELECT data FROM documents WHERE tenant_id=? AND collection=? AND doc_id=?");
        stmt.bind(1, tenant_id);
        stmt.bind(2, collection);
        stmt.bind(3, docId);
        if (stmt.executeStep())
            return json::parse(stmt.getColumn(0).getText());
        return std::nullopt;
    } catch (const SQLite::Exception& e) {
        fmt::print(stderr, "❌ Error DB get ({}/{}/{}): {}\n", tenant_id, collection, docId, e.what());
        return std::nullopt;
    }
}

std::vector<json> Database::list(const std::string& collection, const std::string& tenant_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<json> results;
    try {
        SQLite::Statement stmt(db_,
            "SELECT data FROM documents WHERE tenant_id=? AND collection=?");
        stmt.bind(1, tenant_id);
        stmt.bind(2, collection);
        while (stmt.executeStep()) {
            results.push_back(json::parse(stmt.getColumn(0).getText()));
        }
    } catch (const SQLite::Exception& e) {
        fmt::print(stderr, "❌ Error DB list ({}/{}): {}\n", tenant_id, collection, e.what());
    }
    return results;
}
