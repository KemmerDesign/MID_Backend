#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <mutex>
#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/traits.h>

// Usamos nlohmann para manipular datos JSON fácilmente
using json = nlohmann::json;

class FirebaseClient {
public:
    // Método estático para obtener la única instancia (Singleton)
    static FirebaseClient& getInstance();

    // Eliminar constructores de copia para evitar duplicados
    FirebaseClient(const FirebaseClient&) = delete;
    void operator=(const FirebaseClient&) = delete;

    // --- Métodos Principales ---
    
    /**
     * Guarda o actualiza un documento en una colección.
     * @param collection Nombre de la colección (ej: "suppliers")
     * @param docId ID del documento (ej: "1")
     * @param data Objeto JSON con los datos a guardar
     * @return true si tuvo éxito, false si falló
     */
    bool addDocument(const std::string& collection, const std::string& docId, const json& data);
    // ... (dentro de la clase FirebaseClient, sección public)

    // Nuevo método para LEER
    json getDocument(const std::string& collection, const std::string& docId);

private:
    // Constructor privado (se llama automáticamente al inicio)
    FirebaseClient();

    // Carga el archivo service-account.json del disco
    void loadCredentials();

    // Genera un Token de Acceso válido negociando con Google
    std::string getAccessToken();

    // Variables internas de configuración
    std::string projectId_;
    std::string clientEmail_;
    std::string privateKey_;
    std::string baseUrl_;
    
    // Cache simple del token para no pedir uno nuevo en cada petición
    std::string currentAccessToken_;
};