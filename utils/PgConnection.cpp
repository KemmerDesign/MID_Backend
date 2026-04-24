#include "PgConnection.h"
#include <fmt/core.h>
#include <cstdlib>
#include <stdexcept>

PgConnection& PgConnection::getInstance() {
    static PgConnection instance;
    return instance;
}

PgConnection::PgConnection() : conn_(nullptr, &PQfinish) {
    connect();
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
    conn_.reset(PQconnectdb(buildConnStr().c_str()));

    if (PQstatus(conn_.get()) != CONNECTION_OK) {
        std::string err = PQerrorMessage(conn_.get());
        conn_.reset();
        throw std::runtime_error("PgConnection: no se pudo conectar a PostgreSQL — " + err);
    }

    fmt::print("🐘 PostgreSQL conectado: {}\n", PQdb(conn_.get()));
}

bool PgConnection::isConnected() const {
    return conn_ && PQstatus(conn_.get()) == CONNECTION_OK;
}

PgResult PgConnection::exec(const std::string& sql,
                             const std::vector<std::string>& params) {
    if (!isConnected()) {
        PQreset(conn_.get());
        if (!isConnected())
            throw std::runtime_error("PgConnection: conexión perdida y no se pudo recuperar");
    }

    if (params.empty())
        return PgResult(PQexec(conn_.get(), sql.c_str()));

    std::vector<const char*> cParams;
    cParams.reserve(params.size());
    for (const auto& p : params) cParams.push_back(p.c_str());

    return PgResult(PQexecParams(
        conn_.get(),
        sql.c_str(),
        static_cast<int>(cParams.size()),
        nullptr,
        cParams.data(),
        nullptr,
        nullptr,
        0
    ));
}
