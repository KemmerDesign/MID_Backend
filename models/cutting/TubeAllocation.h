#pragma once
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

namespace models::cutting {

// Un corte ya asignado a un tubo específico.
struct AllocatedCut {
    double      length_mm;
    std::string label;

    nlohmann::json toJson() const {
        return {{"length_mm", length_mm}, {"label", label}};
    }
};

// Representa un tubo físico con sus cortes asignados y el sobrante resultante.
//
// Física del corte:
//   Al colocar k piezas en un tubo, se realizan k aserradas, cada una consume
//   'blade_kerf_mm' de material. Por tanto:
//     espacio_ocupado = sum(piezas) + k * kerf
//     sobrante        = tube_length - espacio_ocupado
//
//   Condición para que un corte 'c' quepa en el espacio restante 'R':
//     c + kerf <= R
struct TubeAllocation {
    int    tube_number    = 0;
    double tube_length_mm = 6000.0;
    double blade_kerf_mm  = 4.0;

    std::vector<AllocatedCut> cuts;
    double remaining_mm   = 0.0; // espacio disponible durante el empaque
    double remnant_mm     = 0.0; // sobrante final (útil para reusar en otro proyecto)
    double efficiency_pct = 0.0; // % de tubo aprovechado

    TubeAllocation(int number, double tube_len, double kerf)
        : tube_number(number)
        , tube_length_mm(tube_len)
        , blade_kerf_mm(kerf)
        , remaining_mm(tube_len)
    {}

    bool canFit(double cut_length) const {
        return (cut_length + blade_kerf_mm) <= remaining_mm;
    }

    void addCut(const AllocatedCut& cut) {
        cuts.push_back(cut);
        remaining_mm -= (cut.length_mm + blade_kerf_mm);
    }

    // Llamar al finalizar el empaque para calcular sobrante y eficiencia.
    void finalize() {
        remnant_mm     = remaining_mm;
        double used    = tube_length_mm - remnant_mm;
        efficiency_pct = (tube_length_mm > 0) ? (used / tube_length_mm) * 100.0 : 0.0;
    }

    nlohmann::json toJson() const {
        nlohmann::json cuts_json = nlohmann::json::array();
        for (const auto& c : cuts) cuts_json.push_back(c.toJson());

        return {
            {"tube_number",    tube_number},
            {"tube_length_mm", tube_length_mm},
            {"cuts",           cuts_json},
            {"cuts_count",     static_cast<int>(cuts.size())},
            {"remnant_mm",     remnant_mm},
            {"efficiency_pct", efficiency_pct}
        };
    }
};

} // namespace models::cutting
