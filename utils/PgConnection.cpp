#include "PgConnection.h"
#include <fmt/core.h>
#include <cstdlib>
#include <stdexcept>

PgConnection& PgConnection::getInstance() {
    static PgConnection instance;
    return instance;
}

PgConnection::PgConnection() {
    connect();
}

PgConnection::~PgConnection() {
    if (conn_) {
        PQfinish(conn_);
        conn_ = nullptr;
    }
}

std::string PgConnection::buildConnStr() {
    // Si el usuario define MONARCA_PG_CONNSTR completo, lo usamos directamente.
    if (const char* full = std::getenv("MONARCA_PG_CONNSTR")) {
        return full;
    }

    auto env = [](const char* key, const char* def) -> std::string {
        const char* v = std::getenv(key);
        return v ? v : def;
    };

    return fmt::format("host={} port={} dbname={} user={} password={} connect_timeout=5",
        env("MONARCA_PG_HOST",     "localhost"),
        env("MONARCA_PG_PORT",     "5432"),
        env("MONARCA_PG_DB",       "monarca_db"),
        env("MONARCA_PG_USER",     "monarca"),
        env("MONARCA_PG_PASSWORD", "monarca_dev_2024")
    );
}

void PgConnection::connect() {
    std::string connStr = buildConnStr();
    conn_ = PQconnectdb(connStr.c_str());

    if (PQstatus(conn_) != CONNECTION_OK) {
        std::string err = PQerrorMessage(conn_);
        PQfinish(conn_);
        conn_ = nullptr;
        throw std::runtime_error("PgConnection: no se pudo conectar a PostgreSQL — " + err);
    }

    fmt::print("🐘 PostgreSQL conectado: {}\n", PQdb(conn_));
}

bool PgConnection::isConnected() const {
    return conn_ && PQstatus(conn_) == CONNECTION_OK;
}

PgResult PgConnection::exec(const std::string& sql,
                             const std::vector<std::string>& params) {
    // Reconectar si la conexión se perdió (ej. restart del servidor)
    if (!isConnected()) {
        PQreset(conn_);
        if (!isConnected()) {
            throw std::runtime_error("PgConnection: conexión perdida y no se pudo recuperar");
        }
    }

    if (params.empty()) {
        return PgResult(PQexec(conn_, sql.c_str()));
    }

    // Convertir vector<string> → array de const char* para PQexecParams
    std::vector<const char*> cParams;
    cParams.reserve(params.size());
    for (const auto& p : params) {
        cParams.push_back(p.c_str());
    }

    return PgResult(PQexecParams(
        conn_,
        sql.c_str(),
        static_cast<int>(cParams.size()),
        nullptr,      // tipos inferidos por PostgreSQL
        cParams.data(),
        nullptr,      // longitudes (null = tratar como C-string)
        nullptr,      // formatos (null = texto)
        0             // resultado en formato texto
    ));
}
