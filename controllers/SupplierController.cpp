#include "SupplierController.h"
#include "../utils/FirebaseClient.h"  // <--- Importamos nuestro cliente
#include <drogon/utils/Utilities.h>   // <--- Para generar IDs únicos (UUID)
#include <trantor/utils/Date.h>       // <--- Para manejar Fechas y Horas

// Usamos el namespace de nlohmann para facilidad
using json = nlohmann::json;

void SupplierController::createOne(const HttpRequestPtr &req,
                                   std::function<void (const HttpResponsePtr &)> &&callback) {
    try {
        // 1. Obtener el cuerpo de la petición (Raw String)
        std::string body = std::string(req->getBody());
        
        if (body.empty()) {
             auto resp = HttpResponse::newHttpResponse();
             resp->setStatusCode(k400BadRequest);
             resp->setBody("❌ Error: El cuerpo del JSON está vacío");
             callback(resp);
             return;
        }

        // 2. Parsear a Nlohmann JSON
        json data = json::parse(body);

        // --- 🛡️ VALIDACIONES DE NEGOCIO (NUEVO) ---

        // A. Validar que el NIT exista y no esté vacío
        if (!data.contains("nit") || data["nit"].is_null() || 
            (data["nit"].is_string() && data["nit"].get<std::string>().empty())) {
             auto resp = HttpResponse::newHttpResponse();
             resp->setStatusCode(k400BadRequest);
             resp->setBody("❌ Error: El campo 'nit' es obligatorio.");
             callback(resp);
             return;
        }

        // B. Validar Razón Social (legal_name)
        if (!data.contains("legal_name") || data["legal_name"].get<std::string>().empty()) {
             auto resp = HttpResponse::newHttpResponse();
             resp->setStatusCode(k400BadRequest);
             resp->setBody("❌ Error: El campo 'legal_name' (Razón Social) es obligatorio.");
             callback(resp);
             return;
        }

        // C. Validar Estructura de Locations (Si las envían, deben ser un Array)
        if (data.contains("locations") && !data["locations"].is_array()) {
             auto resp = HttpResponse::newHttpResponse();
             resp->setStatusCode(k400BadRequest);
             resp->setBody("❌ Error: El campo 'locations' debe ser una lista (Array) de sedes.");
             callback(resp);
             return;
        }

        // --- ⚙️ PREPARACIÓN DE DATOS ---

        // 3. Generar ID único (UUID v4)
        std::string newId = drogon::utils::getUuid();

        // 4. Agregar Metadata (Fecha de creación automática)
        // Esto guarda la fecha/hora actual en formato string legible DB
        data["created_at"] = trantor::Date::now().toDbStringLocal();
        
        // (Opcional) Aseguramos que el ID también quede dentro del documento
        data["id"] = newId; 

        // 5. ¡LLAMADA A FIREBASE! 🔥
        // Gracias a la actualización recursiva en FirebaseClient, ahora podemos pasar
        // el objeto 'data' completo con sus arrays y objetos anidados.
        bool saved = FirebaseClient::getInstance().addDocument("suppliers", newId, data);

        // 6. Responder al cliente
        auto resp = HttpResponse::newHttpResponse();
        
        if (saved) {
            resp->setStatusCode(k201Created);
            // Devolvemos el JSON completo confirmado
            resp->setBody(data.dump()); 
            resp->setContentTypeCode(CT_APPLICATION_JSON);
        } else {
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("❌ Error guardando en Firebase (Revisa logs del servidor)");
        }
        
        callback(resp);

    } catch (const std::exception &e) {
        // Capturar errores de JSON mal formado
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody(std::string("❌ Excepción procesando JSON: ") + e.what());
        callback(resp);
    }
}

// Implementación de getOne (Lo dejamos listo para la siguiente fase)
void SupplierController::getOne(const HttpRequestPtr &req,
                                std::function<void (const HttpResponsePtr &)> &&callback,
                                std::string &&id) {
    
    #ifdef MONARCA_DEV
        LOG_DEBUG << "🔍 [DEV-LOG] Buscando ID: " << id;
        LOG_DEBUG << "   -> IP Cliente: " << req->peerAddr().toIp();
    #endif
    
    // Llamamos a nuestro cliente para buscar en la nube
    json doc = FirebaseClient::getInstance().getDocument("suppliers", id);

    auto resp = HttpResponse::newHttpResponse();

    if (!doc.is_null() && !doc.empty()) {
        // ¡Éxito! Encontramos el proveedor
        #ifdef MONARCA_DEV
            LOG_INFO << "✅ [DEV-LOG] Documento recuperado: " << doc.dump();
        #endif
        resp->setStatusCode(k200OK);
        resp->setBody(doc.dump());
        resp->setContentTypeCode(CT_APPLICATION_JSON);
    } else {
        // No se encontró o hubo error
        LOG_WARN << "⚠️ Proveedor no encontrado: " << id;
        resp->setStatusCode(k404NotFound);
        json errorBody = {
            {"error", "Proveedor no encontrado"},
            {"id", id}
        };
        resp->setBody(errorBody.dump());
        resp->setContentTypeCode(CT_APPLICATION_JSON);
    }

    callback(resp);
}