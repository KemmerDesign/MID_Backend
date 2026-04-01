#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace models::cutting {

// Representa un corte solicitado por el analista.
// 'quantity' indica cuántas piezas de esa longitud se necesitan por unidad de producto.
struct CutRequest {
    double length_mm = 0.0;
    int    quantity  = 1;
    std::string label;      // Ej: "pata frontal", "travesaño"

    nlohmann::json toJson() const {
        return {
            {"length_mm", length_mm},
            {"quantity",  quantity},
            {"label",     label}
        };
    }

    static CutRequest fromJson(const nlohmann::json& j) {
        CutRequest r;
        r.length_mm = j.at("length_mm").get<double>();
        r.quantity  = j.value("quantity", 1);
        r.label     = j.value("label", "");
        return r;
    }
};

} // namespace models::cutting
