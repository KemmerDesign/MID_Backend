#include <drogon/drogon.h>
#include <fmt/core.h>

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

    app().registerHandler("/", [](const HttpRequestPtr&,
                                  std::function<void (const HttpResponsePtr &)> &&callback) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setBody("Hola! Backend C++ corriendo correctamente.");
        callback(resp);
    });

    fmt::print("Escuchando en http://0.0.0.0:8080\n");
    app().addListener("0.0.0.0", 8080).run();
    return 0;
}
