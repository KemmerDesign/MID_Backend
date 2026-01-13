#include "SupplierController.h"
#include <vector>
#include <mutex>

// --- SIMULACIÓN DE BASE DE DATOS ---
static std::vector<models::Supplier> mockDb;
static std::mutex dbMutex;

void SupplierController::createSupplier(const HttpRequestPtr &req,
                                        std::function<void (const HttpResponsePtr &)> &&callback) {
    
    // --- CAMBIO 1: LEER TEXTO PURO ---
    // En lugar de req->getJsonObject() (que devuelve JsonCpp), leemos el string crudo
    std::string requestBody = std::string(req->getBody());

    if (requestBody.empty()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Error: Empty body");
        callback(resp);
        return;
    }

    try {
        
        auto jsonNlohmann = nlohmann::json::parse(requestBody);//hay un parseo explicito para nlohmann
        
        models::Supplier newSupplier = models::Supplier::fromJson(jsonNlohmann);
        {
            std::lock_guard<std::mutex> lock(dbMutex);
            if(newSupplier.getId() == 0) {
                newSupplier.setId(static_cast<int>(mockDb.size()) + 1);
            }
            mockDb.push_back(newSupplier);
        }

        auto resp = HttpResponse::newHttpResponse(); 
        resp->setBody(newSupplier.toJson().dump()); // .dump() convierte JSON a string
        resp->setStatusCode(k201Created);
        resp->setContentTypeCode(CT_APPLICATION_JSON); // Le decimos al navegador que es JSON
        callback(resp);

    } catch (const std::exception &e) {
        LOG_ERROR << "Error creating supplier: " << e.what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest); // 400 porque probablemente falló el parseo
        resp->setBody("Error: Invalid JSON format");
        callback(resp);
    }
}

void SupplierController::getSuppliers(const HttpRequestPtr &req,
                                      std::function<void (const HttpResponsePtr &)> &&callback) {
    
    nlohmann::json result = nlohmann::json::array();

    {
        std::lock_guard<std::mutex> lock(dbMutex);
        for (const auto& supplier : mockDb) {
            result.push_back(supplier.toJson());
        }
    }
    auto resp = HttpResponse::newHttpResponse();
    resp->setBody(result.dump()); // Convertimos el array JSON a String
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    callback(resp);
}