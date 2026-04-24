#pragma once
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <libpq-fe.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Excepción que transporta el SQLSTATE de PostgreSQL (5 chars, ej. "23505").
// Permite a los controladores mapear errores de DB a códigos HTTP precisos
// sin exponer nombres de tablas ni columnas al cliente.
struct PgException : std::runtime_error {
    std::string sqlstate;
    PgException(const std::string& msg, const std::string& state)
        : std::runtime_error(msg), sqlstate(state) {}
};

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

    // SQLSTATE de 5 caracteres (ej. "23505" = unique_violation).
    // Vacío si no hay resultado o si no hubo error.
    std::string pgCode() const {
        if (!res) return "";
        const char* s = PQresultErrorField(res.get(), PG_DIAG_SQLSTATE);
        return s ? s : "";
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
    // Serializa acceso desde los hilos del thread-pool de Drogon.
    // libpq no es thread-safe sobre una misma conexión sin serialización.
    mutable std::mutex mtx_;

    PgConnection();
    ~PgConnection() = default;

    void connect();
    static std::string buildConnStr();
};
