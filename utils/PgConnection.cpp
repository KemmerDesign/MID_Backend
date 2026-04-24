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
    if (const char* full = std::getenv("MONARCA_PG_CONNSTR")) return full;

    // En desarrollo: fallback a valores locales conocidos.
    // En producción: abort si falta la variable — nunca arrancar sin credenciales reales.
    auto reqEnv = [](const char* key, const char* devFallback) -> std::string {
#ifdef MONARCA_DEV
        const char* v = std::getenv(key);
        return v ? v : devFallback;
#else
        const char* v = std::getenv(key);
        if (!v) {
            fmt::print(stderr,
                "❌ [PROD] Variable de entorno '{}' no definida. "
                "El servidor no arranca sin credenciales de base de datos.\n", key);
            std::abort();
        }
        return v;
#endif
    };

    return fmt::format("host={} port={} dbname={} user={} password={} connect_timeout=5",
        reqEnv("MONARCA_PG_HOST",     "localhost"),
        reqEnv("MONARCA_PG_PORT",     "5432"),
        reqEnv("MONARCA_PG_DB",       "monarca_db"),
        reqEnv("MONARCA_PG_USER",     "monarca"),
        reqEnv("MONARCA_PG_PASSWORD", "monarca_dev_2024")
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
    // Un único mutex serializa TODAS las llamadas a libpq sobre esta conexión.
    // libpq no es thread-safe en una sola PGconn; sin esto, dos handlers
    // concurrentes de Drogon causarían corrupción o segfault.
    std::lock_guard<std::mutex> lock(mtx_);

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
