#pragma once
#include <optional>
#include <nlohmann/json.hpp>
#include "MetalMaterial.h" // Incluimos para reutilizar el enum Materialtype

namespace rawMaterial
{
    // Clase modelo para Láminas / Placas / Chapas
    class MetalSheet
    {
    private:
        // Atributos dimensionales
        double lengthMM;
        double widthMM;
        double thicknessMM;

        // Atributos derivados (Cálculos físicos)
        double weightGrams;
        double areaMM2; // Área superficial (útil para pintura o recubrimientos)

        // Propiedades del material
        Materialtype material;
        double density;

        // Constructor privado (se accede vía createMetalSheet)
        MetalSheet(double lengthMM, double widthMM, double thicknessMM, Materialtype material, double density);

        // Métodos privados para lógica interna
        void setWeightGrams(double weight);
        void setAreaMM2(double area);

    public:
        MetalSheet() = default;

        // Factory Method: Valida dimensiones y calcula peso/área inicial
        static std::optional<MetalSheet> createMetalSheet(double lengthMM, double widthMM, double thicknessMM, Materialtype material, double density);

        // Getters
        double getLengthMM() const;
        double getWidthMM() const;
        double getThicknessMM() const;
        double getWeightGrams() const;
        double getAreaMM2() const;
        Materialtype getMaterial() const;
        double getDensity() const;

        // Setters
        void setLengthMM(double length);
        void setWidthMM(double width);
        void setThicknessMM(double thickness);
        void setMaterial(Materialtype material);
        void setDensity(double density);

        // Serialization
        nlohmann::json toJson() const;
        static std::optional<MetalSheet> fromJson(const nlohmann::json& jsonObj);
    };
}
