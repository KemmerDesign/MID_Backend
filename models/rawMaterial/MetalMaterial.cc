#include "MetalMaterial.h"

namespace rawMaterial{
    //Constructor y setters privados y publicos
    MetalProfile::MetalProfile(double lengthMM, double widthMM, double heightMM, double thicknessMM, GeometryType geometry, Materialtype material, double density)
        : lengthMM(lengthMM), widthMM(widthMM), heightMM(heightMM), thicknessMM(thicknessMM), geometry(geometry), material(material), density(density)
    {
        // Calcular area y peso al momento de la creacion
        double area = 0.0;
        double weight = 0.0;

        switch (geometry)
        {
        case GeometryType::CYLINDRICAL:
            area = 3.1416 * thicknessMM * lengthMM; // Area lateral del cilindro
            weight = area * density; // Peso = Area * Densidad
            break;
        case GeometryType::RECTANGULAR:
            area = 2 * (widthMM * heightMM + widthMM * lengthMM + heightMM * lengthMM);
            weight = area * density;
            break;
        case GeometryType::SQUARED:
            area = 4 * (thicknessMM * lengthMM + thicknessMM * heightMM);
            weight = area * density;
            break;
        default:
            break;
        }

        setAreaMM2(area);
        setWeightGrams(weight);
    };
    void MetalProfile::setWeightGrams(double weight){
        this->weightGrams = weight;
    };
    void MetalProfile::setAreaMM2(double area){
        this->areaMM2 = area;
    };
    // TODO: Implementacion de los Setters
    // TODO: Factory method
    // Implementacion de los Getters
    double MetalProfile::getLengthMM() const {
        return lengthMM;
    } 
    double MetalProfile::getWidthMM() const {
        return widthMM;
    }
    double MetalProfile::getHeightMM() const {
        return heightMM;
    }
    double MetalProfile::getThicknessMM() const {
        return thicknessMM;
    }
    double MetalProfile::getWeightGrams() const {
        return weightGrams;
    }
    double MetalProfile::getAreaMM2() const {
        return areaMM2;
    }  
    // TODO: Implementacion serializacion y deserializacion 
}