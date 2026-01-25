#include "MetalMaterial.h"
#include <cmath>    // Para pow() y M_PI

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
    
    // Implementacion de los Setters
    void MetalProfile::setLengthMM(double length) {
        this->lengthMM = length;
    }
    void MetalProfile::setWidthMM(double width) {
        this->widthMM = width;
    }
    void MetalProfile::setHeightMM(double height) {
        this->heightMM = height;
    }
    void MetalProfile::setThicknessMM(double thickness) {
        this->thicknessMM = thickness;
    }
    void MetalProfile::setGeometry(GeometryType geometry) {
        this->geometry = geometry;
    }
    void MetalProfile::setMaterial(Materialtype material) {
        this->material = material;
    }
    void MetalProfile::setDensity(double density) {
        this->density = density;
    }
    
    std::optional<MetalProfile> MetalProfile::createMetalProfile(
        double lengthMM, double widthMM, double heightMM, double thicknessMM, 
        GeometryType geometry, Materialtype material, double density) 
    {
        // --- A. VALIDACIONES DE SEGURIDAD ---
        // Nada de dimensiones negativas o cero
        if (lengthMM <= 0 || widthMM <= 0 || thicknessMM <= 0 || density <= 0) {
            return std::nullopt;
        }
        // Si es rectangular, la altura también importa
        if (geometry == GeometryType::RECTANGULAR && heightMM <= 0) {
            return std::nullopt;
        }

        // --- B. CREACIÓN DEL OBJETO ---
        // Usamos el constructor privado
        MetalProfile profile(lengthMM, widthMM, heightMM, thicknessMM, geometry, material, density);

        // --- C. CÁLCULOS FÍSICOS (Volumen de Material y Área Superficial) ---
        double materialVolumeMM3 = 0.0; // Solo el metal (restando el hueco)
        double paintAreaMM2 = 0.0;      // Superficie exterior

        // Constante PI para C++20
        #ifndef M_PI
        #define M_PI 3.14159265358979323846
        #endif
        const double PI = M_PI; 

        switch (geometry) {
            case GeometryType::CYLINDRICAL: {
                // Tubo Redondo (Hollow Cylinder)
                double outerRadius = widthMM / 2.0;
                double innerRadius = outerRadius - thicknessMM;
                
                // Evitar radios negativos (si el espesor es absurdo)
                if (innerRadius < 0) return std::nullopt;

                double areaCrossSection = (PI * std::pow(outerRadius, 2)) - (PI * std::pow(innerRadius, 2));
                
                materialVolumeMM3 = areaCrossSection * lengthMM;
                paintAreaMM2 = 2 * PI * outerRadius * lengthMM; // Perímetro * Largo
                break;
            }
            case GeometryType::SQUARED: {
                // Tubo Cuadrado
                // Asumimos que widthMM es el lado exterior
                double innerSide = widthMM - (2 * thicknessMM);
                
                if (innerSide < 0) return std::nullopt;

                double areaOuter = widthMM * widthMM;
                double areaInner = innerSide * innerSide;
                
                materialVolumeMM3 = (areaOuter - areaInner) * lengthMM;
                paintAreaMM2 = (4 * widthMM) * lengthMM; // 4 lados * Largo
                break;
            }
            case GeometryType::RECTANGULAR: {
                // Tubo Rectangular
                double innerWidth = widthMM - (2 * thicknessMM);
                double innerHeight = heightMM - (2 * thicknessMM);

                if (innerWidth < 0 || innerHeight < 0) return std::nullopt;

                double areaOuter = widthMM * heightMM;
                double areaInner = innerWidth * innerHeight;

                materialVolumeMM3 = (areaOuter - areaInner) * lengthMM;
                paintAreaMM2 = (2 * widthMM + 2 * heightMM) * lengthMM; // Perímetro * Largo
                break;
            }
        }

        // --- D. ASIGNACIÓN FINAL ---
        // Peso = Volumen del Material * Densidad
        profile.setWeightGrams(materialVolumeMM3 * density);
        profile.setAreaMM2(paintAreaMM2);

        return profile;
    }
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
    
    // Implementacion serializacion y deserializacion 
    nlohmann::json MetalProfile::toJson() const {
        return nlohmann::json{
            {"lengthMM", lengthMM},
            {"widthMM", widthMM},
            {"heightMM", heightMM},
            {"thicknessMM", thicknessMM},
            {"weightGrams", weightGrams},
            {"areaMM2", areaMM2},
            {"geometry", static_cast<int>(geometry)},
            {"material", static_cast<int>(material)},
            {"density", density}
        };
    }

    std::optional<MetalProfile> MetalProfile::fromJson(const nlohmann::json& jsonObj) {
        try {
            return createMetalProfile(
                jsonObj.at("lengthMM").get<double>(),
                jsonObj.at("widthMM").get<double>(),
                jsonObj.at("heightMM").get<double>(),
                jsonObj.at("thicknessMM").get<double>(),
                static_cast<GeometryType>(jsonObj.at("geometry").get<int>()),
                static_cast<Materialtype>(jsonObj.at("material").get<int>()),
                jsonObj.at("density").get<double>()
            );
        } catch (...) {
            return std::nullopt;
        }
    }
}