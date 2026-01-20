#pragma once
#include <optional>

//TODO : Hacer la implementacion para la la serializacion y descerializacion de los objetos de esta clase
//TODO : Hacer mas universalesta clase, para que soporte diferentes tipos de materiales y no solo acero al carbono
namespace rawMaterial
{
    // Declaracion de constantes para operar internamente
    // Las clases deben estar pensadas para operar en mm, es una condicion clave para que sea el mismo lenguaje.
    constexpr double INCH_TO_MM = 25.4;
    constexpr double densityCR = 7.85/1000000; // mm3 * gramos

    enum class Materialtype
    {
        ColdRolled_Steel, // Carbono Regular
        HotRolled_Steel,  // Alta Resistencia
        Aluminum,
        Stanless_Steel
    };

    class MetalProfile
    {
    private:
        //Atributos
        double lengthMM;
        double outerDiameterMM;
        double thicknessMM;
        bool isSquared;
        bool isRectangular;
        bool isCylindrical;
        bool isCR;
        bool isHR;
        //Metodos privados
        MetalProfile(double lengthMM, double outerDiameterMM, double thicknessMM, bool isSquared, bool isRectangular, bool isCylindrical,  bool isCR, bool isHR);
    public:
        MetalProfile();
        //Este constructor es para crear tubos de acero al carbono, se usa el patron Factory Method ya que es mas seguro
        //adicional, si un insumo se va a crear solo puede tener un tipo de geometria, y un tipo de insumo de material.
        //si entraran parametros invalidos, se retornaria un std::nullopt
        std::optional<MetalProfile> static pipeCR_HR_Constructor(double lengthMM, double outerDiameterMM, double thicknessMM, bool isSquared, bool isRectangular, bool isCylindrical);        double getLengthMM() const;
        double getOuterDiameterMM() const;
        double getThicknessMM() const;
        double getInnerDiameterMM() const;
        double calculateVolumeMM3() const;
        double calculateWeightGrams() const;
        bool getIsSquared() const;
        bool getIsRectangular() const;
        bool getIsCylindrical() const;
    };
}