#include "MetalMaterialController.h"
#include "../../utils/FirebaseClient.h"
#include <drogon/utils/Utilities.h>
#include <trantor/utils/Date.h>
#include <string>
#include <sstream> // Para construir el ID string

using json = nlohmann::json;
using namespace rawMaterial; 

void MetalMaterialController::addPipeToStock(const HttpRequestPtr &req,
                                             std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        // 1. Parsing y Validaciones (Igual que antes...)
        std::string body = std::string(req->getBody());
        if (body.empty()) {
            auto resp = HttpResponse::newHttpResponse(); resp->setStatusCode(k400BadRequest);
            resp->setBody("❌ Error: Body vacío"); callback(resp); return;
        }

        json data = json::parse(body);

        // ... (Tus validaciones de existencia y tipos van aquí, las omito para no repetir todo el bloque anterior) ...
        // ASEGURATE DE MANTENER TUS IFs DE VALIDACIÓN AQUÍ

        // --- 🚀 LÓGICA DE NEGOCIO AVANZADA ---

        // 2. Extraer datos limpios
        double length = data["lengthMM"].get<double>();
        double width = data["widthMM"].get<double>();
        double height = data.value("heightMM", 0.0); // Puede ser 0 si es redondo
        double thickness = data["thicknessMM"].get<double>();
        int geoInt = data["geometry"].get<int>();
        int matInt = data["material"].get<int>();
        double density = data["density"].get<double>();
        
        // Cantidad que está entrando ahora a la bodega
        int incomingQty = data.value("quantity", 1); 

        // 3. Generar el SKU (ID Único Determinista)
        // Formato: GEO-MAT-L-W-H-T (Ej: 1-0-6000-100-50-2)
        // Esto garantiza que si metes el mismo tubo, genere el mismo ID.
        std::stringstream ss;
        ss << geoInt << "-" << matInt << "-" << (int)length << "-" 
           << (int)width << "-" << (int)height << "-" << thickness;
        std::string skuId = ss.str();

        // 4. Consultar si ya existe en Firebase
        json existingDoc = FirebaseClient::getInstance().getDocument("stock_inventory", skuId);

        json finalDoc;
        bool isUpdate = false;

        if (existingDoc != nullptr && !existingDoc.empty() && !existingDoc.contains("error")) {
            // A. EL MATERIAL YA EXISTE -> ACTUALIZAR (UPDATE)
            isUpdate = true;
            
            // Recuperamos la cantidad actual y sumamos la nueva
            int currentQty = existingDoc.value("quantity_on_hand", 0);
            int newTotal = currentQty + incomingQty;
            
            // Mantenemos los datos existentes, solo actualizamos lo que cambia
            finalDoc = existingDoc; 
            finalDoc["quantity_on_hand"] = newTotal;
            finalDoc["updated_at"] = trantor::Date::now().toDbStringLocal();

            // Lógica de reservas (Si existía, mantenemos su valor, si no, es 0)
            // No tocamos 'reserved_quantity' aquí, porque solo estamos metiendo stock, no vendiendo.
        } 
        else {
            // B. ES MATERIAL NUEVO -> CREAR (CREATE)
            
            // Llamamos al Factory para cálculos físicos (Peso, Área)
            auto profileOpt = MetalProfile::createMetalProfile(
                length, width, height, thickness,
                static_cast<GeometryType>(geoInt),
                static_cast<Materialtype>(matInt),
                density
            );

            if (!profileOpt.has_value()) {
                auto resp = HttpResponse::newHttpResponse(); resp->setStatusCode(k400BadRequest);
                resp->setBody("❌ Error Físico: Dimensiones imposibles."); callback(resp); return;
            }

            // Convertimos a JSON base
            finalDoc = profileOpt.value().toJson();
            
            // Agregamos los campos de gestión de inventario
            finalDoc["id"] = skuId; // El ID es el SKU
            finalDoc["sku"] = skuId; // Guardamos el SKU visiblemente
            finalDoc["quantity_on_hand"] = incomingQty;
            finalDoc["reserved_quantity"] = 0; // Inicializamos reservas en 0
            finalDoc["created_at"] = trantor::Date::now().toDbStringLocal();
            finalDoc["updated_at"] = trantor::Date::now().toDbStringLocal();
        }

        // 5. Guardar en Firebase (Upsert: Crea o Sobreescribe)
        bool saved = FirebaseClient::getInstance().addDocument("stock_inventory", skuId, finalDoc);

        auto resp = HttpResponse::newHttpResponse();
        if (saved) {
            resp->setStatusCode(isUpdate ? k200OK : k201Created);
            
            // Respondemos con el estado actual del inventario
            json responseBody;
            responseBody["message"] = isUpdate ? "Stock actualizado" : "Material nuevo creado";
            responseBody["sku"] = skuId;
            responseBody["current_stock"] = finalDoc["quantity_on_hand"];
            responseBody["reserved"] = finalDoc["reserved_quantity"];
            
            resp->setBody(responseBody.dump());
            resp->setContentTypeCode(CT_APPLICATION_JSON);
        } else {
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("❌ Error guardando en base de datos.");
        }

        callback(resp);
    }
    catch (const std::exception &e)
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody(std::string("❌ Excepción interna: ") + e.what());
        callback(resp);
    }
}