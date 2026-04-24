#include <drogon/drogon.h>
#include <fmt/core.h>
#include "controllers/AuthController.h"
#include "controllers/CommercialController.h"
#include "controllers/ProjectController.h"
#include "controllers/ProductCatalogController.h"
#include "controllers/EmployeeController.h"
#include "controllers/RolesController.h"
#include "utils/PgConnection.h"

using namespace drogon;

int main() {

    //Configuracion de LOGS
    #ifdef MONARCA_DEV
        // MODO DESARROLLO: Ver todo (Debug, Info, Warn, Error)
        trantor::Logger::setLogLevel(trantor::Logger::kDebug);
        LOG_INFO << "🐛 MODO DESARROLLO ACTIVADO: Logs detallados encendidos";
    #else
        // MODO PRODUCCIÓN: Solo ver Advertencias y Errores (Ahorra CPU)
        trantor::Logger::setLogLevel(trantor::Logger::kWarn);
        // No imprimimos nada aquí para mantener la consola limpia
    #endif

    fmt::print("🚀 Iniciando servidor C++ con Clang...\n");

    // Inicializar conexión a PostgreSQL al arrancar (falla rápido si no hay servidor)
    try {
        PgConnection::getInstance();
    } catch (const std::exception& e) {
        fmt::print(stderr, "❌ Error conectando a PostgreSQL: {}\n", e.what());
        return 1;
    }

    app().registerHandler("/", [](const HttpRequestPtr&,
                                  std::function<void (const HttpResponsePtr &)> &&callback) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setBody("Hola! Backend C++ corriendo correctamente.");
        callback(resp);
    });

    // CORS: responder preflight OPTIONS y agregar headers a toda respuesta
    app().registerSyncAdvice([](const HttpRequestPtr& req) -> HttpResponsePtr {
        if (req->method() == Options) {
            auto resp = HttpResponse::newHttpResponse();
            resp->addHeader("Access-Control-Allow-Origin",  "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age",       "86400");
            resp->setStatusCode(k204NoContent);
            return resp;
        }
        return nullptr;
    });

    app().registerPostHandlingAdvice([](const HttpRequestPtr&, const HttpResponsePtr& resp) {
        resp->addHeader("Access-Control-Allow-Origin",  "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    });

    fmt::print("Escuchando en http://0.0.0.0:8080\n");
    app().addListener("0.0.0.0", 8080).run();
    return 0;
}
