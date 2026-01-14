#include "FirebaseClient.h"
#include <cpr/cpr.h>
#include <jwt-cpp/jwt.h> // Librería para firmar tokens
#include <fstream>
#include <iostream>
#include <chrono>
#include <fmt/core.h> // Usamos fmt para logs limpios (viene con Drogon)

FirebaseClient& FirebaseClient::getInstance() {
    static FirebaseClient instance;
    return instance;
}

FirebaseClient::FirebaseClient() {
    loadCredentials();
}

void FirebaseClient::loadCredentials() {
    // Ruta donde guardaste la llave con gcloud
    std::string path = "service-account.json"; // Busca en la misma carpeta del ejecutable (build/)
    
    std::ifstream file(path);
    if (!file.is_open()) {
        // Intentamos ruta absoluta relativa a build si falla la primera
        file.open("build/service-account.json");
    }

    if (!file.is_open()) {
        fmt::print(stderr, "❌ ERROR FATAL: No se encontró 'service-account.json'.\n");
        fmt::print(stderr, "   Asegúrate de haber generado la llave en la carpeta build/.\n");
        return;
    }

    try {
        json creds = json::parse(file);
        
        // Extraemos los datos necesarios del archivo de Google
        projectId_ = creds.at("project_id");
        clientEmail_ = creds.at("client_email");
        privateKey_ = creds.at("private_key");

        // URL base para la API REST de Firestore
        baseUrl_ = "https://firestore.googleapis.com/v1/projects/" + projectId_ + "/databases/(default)/documents/";

        fmt::print("🔐 Cliente Firebase inicializado para: {}\n", clientEmail_);

    } catch (const std::exception& e) {
        fmt::print(stderr, "❌ Error leyendo JSON de credenciales: {}\n", e.what());
    }
}

//TODO Arreglar el error de compilación aquí
std::string FirebaseClient::getAccessToken() {
    if (!currentAccessToken_.empty()) {
        return currentAccessToken_;
    }

    auto now = std::chrono::system_clock::now();

    // 1. Crear el JWT
    // CORRECCIÓN: Usamos jwt::create() vacío. La librería detectará picojson sola.
    auto token = jwt::create()
        .set_issuer(clientEmail_)
        .set_type("JWT")
        .set_audience("https://oauth2.googleapis.com/token")
        // CORRECCIÓN CLAVE: Pasamos el string DIRECTAMENTE.
        // Borramos "jwt::claim(...)" porque eso causaba el error de compilación.
        .set_payload_claim("scope", "https://www.googleapis.com/auth/datastore")
        .set_issued_at(now)
        .set_expires_at(now + std::chrono::hours(1))
        .sign(jwt::algorithm::rs256("", privateKey_, "", ""));

    // 2. Intercambiar por Access Token (Igual que antes)
    cpr::Response r = cpr::Post(
        cpr::Url{"https://oauth2.googleapis.com/token"},
        cpr::Payload{
            {"grant_type", "urn:ietf:params:oauth:grant-type:jwt-bearer"},
            {"assertion", token}
        }
    );

    if (r.status_code == 200) {
        // Parseamos la respuesta de Google
        // Usamos picojson para leer la respuesta porque jwt-cpp ya lo incluyó
        auto jsonResp = json::parse(r.text);
        currentAccessToken_ = jsonResp["access_token"];
        return currentAccessToken_;
    } else {
        fmt::print(stderr, "❌ Fallo al autenticar con Google: {}\n", r.text);
        return "";
    }
}

// Helper: Firestore requiere un formato JSON específico con tipos explícitos
// Ej: {"nombre": "Juan"} -> {"fields": {"nombre": {"stringValue": "Juan"}}}
json convertToFirestoreJson(const json& data) {
    json fields;
    for (auto& [key, val] : data.items()) {
        if (val.is_string()) {
            fields[key] = { {"stringValue", val} };
        } else if (val.is_number_integer()) {
            fields[key] = { {"integerValue", val} };
        } else if (val.is_number_float()) {
            fields[key] = { {"doubleValue", val} };
        } else if (val.is_boolean()) {
            fields[key] = { {"booleanValue", val} };
        }
    }
    return { {"fields", fields} };
}

bool FirebaseClient::addDocument(const std::string& collection, const std::string& docId, const json& data) {
    // 1. Obtener Token
    std::string token = getAccessToken();
    if (token.empty()) return false;

    // 2. Preparar URL y Datos
    // Usamos PATCH para "Upsert" (Crear o Actualizar)
    std::string url = baseUrl_ + collection + "/" + docId;
    json firestoreBody = convertToFirestoreJson(data);

    // 3. Enviar Petición
    cpr::Response r = cpr::Patch(
        cpr::Url{url},
        cpr::Header{
            {"Authorization", "Bearer " + token},
            {"Content-Type", "application/json"}
        },
        cpr::Body{firestoreBody.dump()}
    );

    if (r.status_code == 200) {
        fmt::print("✅ Guardado en Firestore: {}/{}\n", collection, docId);
        return true;
    } else {
        fmt::print(stderr, "❌ Error Firestore ({}): {}\n", r.status_code, r.text);
        return false;
    }
}