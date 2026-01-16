#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace utils {

    class Adress {
    private:
        std::string street;
        std::string city;
        std::string state;
        std::string zip_code;
        std::string country;

    public:
        Adress();
        Adress(const std::string& street, const std::string& city, const std::string& state,
               const std::string& zip_code, const std::string& country);

        // Getters
        std::string getStreet() const;
        std::string getCity() const;
        std::string getState() const;
        std::string getZipCode() const;
        std::string getCountry() const;

        // Setters
        void setStreet(const std::string& street);
        void setCity(const std::string& city);
        void setState(const std::string& state);
        void setZipCode(const std::string& zip_code);
        void setCountry(const std::string& country);

        // Serialization to JSON
        nlohmann::json toJson() const;

        // Deserialization from JSON
        static Adress fromJson(const nlohmann::json& j);
    };

} // namespace utils