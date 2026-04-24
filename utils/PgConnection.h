#pragma once
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <stdexcept>
#include <libpq-fe.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Wrapper RAII para un resultado de consulta PostgreSQL.
// unique_ptr con el deleter de libpq garantiza que PQclear se llame
// exactamente una vez, incluso si se lanza una excepción.
struct PgResult {
    std::unique_ptr<PGresult, decltype(&PQclear)> res;

    explicit PgResult(PGresult* r) : res(r, &PQclear) {}

    PgResult(const PgResult&)            = delete;
    PgResult& operator=(const PgResult&) = delete;
    PgResult(PgResult&&)                 = default;
    PgResult& operator=(PgResult&&)      = default;

    bool ok()     const { return res && PQresultStatus(res.get()) == PGRES_TUPLES_OK; }
    bool cmdOk()  const { return res && PQresultStatus(res.get()) == PGRES_COMMAND_OK; }
    int  rows()   const { return res ? PQntuples(res.get()) : 0; }
    int  cols()   const { return res ? PQnfields(res.get()) : 0; }

    // Devuelve el valor de una celda o "" si es NULL
    std::string val(int row, int col) const {
        if (!res || PQgetisnull(res.get(), row, col)) return "";
        return PQgetvalue(res.get(), row, col);
    }

    std::string val(int row, const char* colName) const {
        int col = PQfnumber(res.get(), colName);
        return (col < 0) ? "" : val(row, col);
    }

    // Mensaje de error de libpq, o texto de fallback si no hay resultado
    const char* errMsg() const {
        return res ? PQresultErrorMessage(res.get()) : "sin resultado";
    }
};

// Singleton de conexión a PostgreSQL (libpq).
// unique_ptr con el deleter PQfinish garantiza cierre limpio sin destructor manual.
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
    std::unique_ptr<PGconn, decltype(&PQfinish)> conn_;

    PgConnection();
    ~PgConnection() = default;

    void connect();
    static std::string buildConnStr();
};
