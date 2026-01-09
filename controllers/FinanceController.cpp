#include "FinanceController.h"
#include <nlohmann/json.hpp> // Usamos la librería que instalamos con vcpkg

using json = nlohmann::json;

void FinanceController::calculateWacc(const HttpRequestPtr &req,
                                      std::function<void (const HttpResponsePtr &)> &&callback) {
    
    // 1. Obtener el cuerpo de la petición (el JSON que envías desde Postman)
    auto bodyStr = req->getBody();
    
    // 2. Parsear el JSON (Convertir texto a objeto C++)
    json input;
    try {
        input = json::parse(bodyStr);
    } catch (const std::exception &e) {
        // Si el JSON está mal formado, devolvemos error 400
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Error: Invalid JSON format");
        callback(resp);
        return;
    }

    // 3. Extraer variables (Validamos que existan)
    // Usamos .value("key", default) para evitar crashes si falta un dato

    double equity = input.value("equity", 0.0);
    double debt   = input.value("debt", 0.0);
    double costEquity = input.value("cost_equity", 0.0); // En decimal (ej: 0.12 para 12%)
    double costDebt   = input.value("cost_debt", 0.0);   // En decimal
    double taxRate    = input.value("tax_rate", 0.0);    // En decimal

    double totalValue = equity + debt;

    if(equity < 0 || debt < 0) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Error: Equity and Debt must be non-negative");
        callback(resp);
        return;
    }


    // 4. Lógica de Negocio (El cálculo del WACC)
    
    
    if (totalValue == 0) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Error: Total value (Equity + Debt) cannot be zero");
        callback(resp);
        return;
    }

    double weightEquity = equity / totalValue;
    double weightDebt = debt / totalValue;

    // Fórmula WACC: (We * Ke) + (Wd * Kd * (1 - T))
    double waccResult = (weightEquity * costEquity) + (weightDebt * costDebt * (1.0 - taxRate));

    // 5. Preparar respuesta
    json output;
    output["project_value"] = totalValue;
    output["wacc_decimal"] = waccResult;
    output["wacc_percentage"] = waccResult * 100.0;
    output["message"] = "Calculation successful";

    auto resp = HttpResponse::newHttpResponse();
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    resp->setBody(output.dump()); // .dump() convierte el objeto JSON a string
    callback(resp);
}

