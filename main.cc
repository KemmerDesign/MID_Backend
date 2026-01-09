#include <drogon/drogon.h>
#include <fmt/core.h>

using namespace drogon;

int main() {
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
