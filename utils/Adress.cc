#include "Adress.h"

#include <string>
#include <vector>
#include <iostream>
#include <nlohmann/json.hpp>

namespace utils {

    Adress::Adress() : street(""), city(""), state(""), zip_code(""), country("") {}

    Adress::Adress(const std::string& street, const std::string& city, const std::string& state,
                   const std::string& zip_code, const std::string& country)
        : street(street), city(city), state(state), zip_code(zip_code), country(country) {}

    // Getters
    std::string Adress::getStreet() const { return street; }
    std::string Adress::getCity() const { return city; }
    std::string Adress::getState() const { return state; }
    std::string Adress::getZipCode() const { return zip_code; }
    std::string Adress::getCountry() const { return country; }

    // Setters
    void Adress::setStreet(const std::string& street) { this->street = street; }
    void Adress::setCity(const std::string& city) { this->city = city; }
    void Adress::setState(const std::string& state) { this->state = state; }
    void Adress::setZipCode(const std::string& zip_code) { this->zip_code = zip_code; }
    void Adress::setCountry(const std::string& country) { this->country = country; }

    // Serialization to JSON
    nlohmann::json Adress::toJson() const {
        nlohmann::json j;
        j["street"] = street;
        j["city"] = city;
        j["state"] = state;
        j["zip_code"] = zip_code;
        j["country"] = country;
        return j;
    }

    // Deserialization from JSON
    Adress Adress::fromJson(const nlohmann::json& j) {
        Adress adress;
        adress.street = j.value("street","");
        adress.city = j.value("city","");
        adress.state = j.value("state","");
        adress.zip_code = j.value("zip_code","");
        adress.country = j.value("country","");
        return adress;
    }

} // namespace utils