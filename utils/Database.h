#pragma once
#include <string>
#include <mutex>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>
#include <SQLiteCpp/SQLiteCpp.h>

using json = nlohmann::json;

class Database {
public:
    static Database& getInstance();

    Database(const Database&)            = delete;
    Database& operator=(const Database&) = delete;

    // tenant_id se incluye en la PK para aislamiento multi-tenant.
    bool save(const std::string& collection, const std::string& docId,
              const json& data, const std::string& tenant_id = "");

    std::optional<json> get(const std::string& collection, const std::string& docId,
                             const std::string& tenant_id = "");

    std::vector<json> list(const std::string& collection, const std::string& tenant_id = "");

private:
    SQLite::Database db_;
    mutable std::mutex mtx_;

    Database();
    void initSchema();
};
