#pragma once
#include <optional>
#include <nlohmann/json.hpp>

// TODO : Hacer la implementacion para la la serializacion y descerializacion de los objetos de esta clase
// TODO : Hacer mas universalesta clase, para que soporte diferentes tipos de materiales y no solo acero al carbono
namespace rawMaterial
{
    // Declaracion de constantes para operar internamente
    // Las clases deben estar pensadas para operar en mm, es una condicion clave para que sea el mismo lenguaje.

    enum class Materialtype
    {
        COLD_ROLLED_STEEL, // Carbono Regular
        HOT_ROLLED_STEEL,  // Alta Resistencia
        ALUMINIUM,
        STAINLESS_STEEL
    };

    enum class GeometryType
    {
        CYLINDRICAL,
        RECTANGULAR,
        SQUARED
    };

    class MetalProfile
    {
    private:
        // Atributos
        double lengthMM;
        double widthMM;
        double heightMM;
        double thicknessMM;
        double weightGrams;
        double areaMM2;
        // Atributos derivados
        GeometryType geometry;
        Materialtype material;
        double density;
        // Metodos privados
        MetalProfile(double lengthMM, double widthMM, double heightMM, double thicknessMM, GeometryType geometry, Materialtype material, double density);
        void setWeightGrams(double weight);
        void setAreaMM2(double area);
    public:
        MetalProfile() = default;
        static std::optional<MetalProfile> createMetalProfile(double lengthMM, double widthMM, double heightMM, double thicknessMM, GeometryType geometry, Materialtype material, double density);
        // Getters
        double getLengthMM() const;
        double getWidthMM() const;
        double getHeightMM() const;
        double getThicknessMM() const;
        double getWeightGrams() const;
        double getAreaMM2() const;
        GeometryType getGeometry() const;
        Materialtype getMaterial() const;
        double getDensity() const;

        // Setters
        void setLengthMM(double length);
        void setWidthMM(double width);
        void setHeightMM(double height);
        void setThicknessMM(double thickness);
        void setGeometry(GeometryType geometry);
        void setMaterial(Materialtype material);
        void setDensity(double density);

        // Serialization
        nlohmann::json toJson() const;
        static std::optional<MetalProfile> fromJson(const nlohmann::json& jsonObj);
    };
}