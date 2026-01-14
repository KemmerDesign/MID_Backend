#include "FirebaseClient.h"
#include <cpr/cpr.h>
#include <jwt-cpp/jwt.h> // Librería para firmar tokens
#include <fstream>
#include <iostream>
#include <chrono>
#include <fmt/core.h> // Usamos fmt para logs limpios (viene con Drogon)


using JsonTraits = jwt::traits::nlohmann_json;

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
    // 1. Verificar si ya tenemos token válido en memoria
    if (!currentAccessToken_.empty()) {
        return currentAccessToken_;
    }

    auto now = std::chrono::system_clock::now();

    // 2. Crear el JWT firmado (Esto lo tenías bien)
    auto token = jwt::create<JsonTraits>()
        .set_issuer(clientEmail_)
        .set_type("JWT")
        .set_audience("https://oauth2.googleapis.com/token")
        .set_payload_claim("scope", "https://www.googleapis.com/auth/datastore")
        .set_issued_at(now)
        .set_expires_at(now + std::chrono::hours(1))
        .sign(jwt::algorithm::rs256("", privateKey_, "", ""));

    // 3. --- TE FALTABA ESTO: Intercambiar el JWT por el Access Token real ---
    // Hacemos la petición POST a Google
    cpr::Response r = cpr::Post(
        cpr::Url{"https://oauth2.googleapis.com/token"},
        cpr::Payload{
            {"grant_type", "urn:ietf:params:oauth:grant-type:jwt-bearer"},
            {"assertion", token} // Enviamos el token que acabamos de firmar
        }
    );

    // 4. Procesar la respuesta
    if (r.status_code == 200) {
        // Parseamos la respuesta de Google
        auto jsonResp = json::parse(r.text);
        currentAccessToken_ = jsonResp["access_token"];
        return currentAccessToken_;
    } else {
        fmt::print(stderr, "❌ Fallo al autenticar con Google: {}\n", r.text);
        return ""; 
    }
    
    return ""; // El paracaídas final
}

// Helper: Firestore requiere un formato JSON específico con tipos explícitos
// Ej: {"nombre": "Juan"} -> {"fields": {"nombre": {"stringValue": "Juan"}}}
json convertToFirestoreJson(const json& data);

// Helper para convertir un VALOR individual al formato loco de Firestore
json convertValueToFirestore(const json& val) {
    if (val.is_string()) {
        return { {"stringValue", val} };
    } 
    else if (val.is_number_integer()) {
        return { {"integerValue", val} };
    } 
    else if (val.is_number_float()) {
        return { {"doubleValue", val} };
    } 
    else if (val.is_boolean()) {
        return { {"booleanValue", val} };
    } 
    else if (val.is_null()) {
        return { {"nullValue", nullptr} };
    }
    else if (val.is_object()) {
        // Recursividad para objetos anidados (Maps)
        return { {"mapValue", convertToFirestoreJson(val)} };
    }
    else if (val.is_array()) {
        // Recursividad para listas (Arrays)
        json values = json::array();
        for (auto& el : val) {
            values.push_back(convertValueToFirestore(el));
        }
        return { {"arrayValue", { {"values", values} } } };
    }
    return {};
}

// Función principal que procesa el JSON completo
json convertToFirestoreJson(const json& data) {
    json fields;
    for (auto& [key, val] : data.items()) {
        fields[key] = convertValueToFirestore(val);
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

// ---------------------------------------------------------
// HELPER: Convertir DE Firestore (Alien) A JSON Normal (Humano)
// ---------------------------------------------------------
json convertFromFirestoreJson(const json& firestoreData) {
    json result;
    
    // Firestore devuelve un objeto raíz con "fields" o directamente el valor tipo
    if (firestoreData.contains("fields")) {
        for (auto& [key, val] : firestoreData["fields"].items()) {
            result[key] = convertFromFirestoreJson(val); // Recursividad
        }
        return result;
    }

    // Casos Primitivos
    if (firestoreData.contains("stringValue")) return firestoreData["stringValue"];
    if (firestoreData.contains("integerValue")) {
        // Firestore a veces manda los enteros como strings, hay que convertir
        std::string numStr = firestoreData["integerValue"];
        return std::stoll(numStr); 
    }
    if (firestoreData.contains("doubleValue")) return firestoreData["doubleValue"];
    if (firestoreData.contains("booleanValue")) return firestoreData["booleanValue"];
    
    // Casos Complejos (Recursivos)
    if (firestoreData.contains("mapValue")) {
        return convertFromFirestoreJson(firestoreData["mapValue"]);
    }
    if (firestoreData.contains("arrayValue")) {
        json arr = json::array();
        // arrayValue tiene dentro "values"
        if (firestoreData["arrayValue"].contains("values")) {
            for (auto& el : firestoreData["arrayValue"]["values"]) {
                arr.push_back(convertFromFirestoreJson(el));
            }
        }
        return arr;
    }

    return nullptr;
}

// ---------------------------------------------------------
// MÉTODO PRINCIPAL: Obtener Documento (GET)
// ---------------------------------------------------------
json FirebaseClient::getDocument(const std::string& collection, const std::string& docId) {
    std::string token = getAccessToken();
    if (token.empty()) return nullptr;

    std::string url = baseUrl_ + collection + "/" + docId;

    cpr::Response r = cpr::Get(
        cpr::Url{url},
        cpr::Header{{"Authorization", "Bearer " + token}}
    );

    if (r.status_code == 200) {
        json raw = json::parse(r.text);
        // Convertimos el formato feo de Firestore a JSON limpio
        return convertFromFirestoreJson(raw);
    } else if (r.status_code == 404) {
        fmt::print("⚠️ Documento no encontrado: {}/{}\n", collection, docId);
        return nullptr; // O un JSON vacío indicando error
    } else {
        fmt::print(stderr, "❌ Error Firestore GET ({}): {}\n", r.status_code, r.text);
        return nullptr;
    }
}