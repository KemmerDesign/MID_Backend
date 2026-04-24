#pragma once
#include <string>
#include <vector>
#include <optional>
#include <stdexcept>
#include <libpq-fe.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Wrapper RAII para un resultado de consulta PostgreSQL
struct PgResult {
    PGresult* res = nullptr;

    explicit PgResult(PGresult* r) : res(r) {}
    ~PgResult() { if (res) PQclear(res); }

    PgResult(const PgResult&)            = delete;
    PgResult& operator=(const PgResult&) = delete;
    PgResult(PgResult&& other) noexcept : res(other.res) { other.res = nullptr; }

    bool ok()     const { return res && PQresultStatus(res) == PGRES_TUPLES_OK; }
    bool cmdOk()  const { return res && PQresultStatus(res) == PGRES_COMMAND_OK; }
    int  rows()   const { return res ? PQntuples(res) : 0; }
    int  cols()   const { return res ? PQnfields(res) : 0; }

    // Devuelve el valor de una celda o "" si es NULL
    std::string val(int row, int col) const {
        if (!res || PQgetisnull(res, row, col)) return "";
        return PQgetvalue(res, row, col);
    }

    std::string val(int row, const char* colName) const {
        int col = PQfnumber(res, colName);
        return (col < 0) ? "" : val(row, col);
    }
};

// Singleton de conexión a PostgreSQL (libpq)
// Lee la cadena de conexión de la variable de entorno MONARCA_PG_CONNSTR,
// con fallback a credenciales de desarrollo.
class PgConnection {
public:
    static PgConnection& getInstance();

    PgConnection(const PgConnection&)            = delete;
    PgConnection& operator=(const PgConnection&) = delete;

    // Ejecuta una consulta con parámetros ligados ($1, $2, ...) — nunca concatenar.
    // Lanza std::runtime_error si la conexión está caída.
    PgResult exec(const std::string& sql,
                  const std::vector<std::string>& params = {});

    bool isConnected() const;

private:
    PGconn* conn_ = nullptr;

    PgConnection();
    ~PgConnection();

    void connect();
    static std::string buildConnStr();
};
