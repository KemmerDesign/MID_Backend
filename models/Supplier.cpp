#include "Supplier.h"
#include <iostream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>


namespace models
{

    
    Supplier::Supplier() = default;
    Supplier::Supplier(int id, std::string name, std::string address, std::string phone, std::string email)
        : id(id), name(name), address(address), phone(phone), email(email) {}

    // Getters
    int Supplier::getId() const { return id; }
    std::string Supplier::getAddress() const { return address; }
    std::string Supplier::getPhone() const { return phone; }
    std::string Supplier::getEmail() const { return email; }
    std::string Supplier::getName() const { return name; } // Cuidado: Arriba usabas getName, asegúrate de ser consistente

    // Setters
    void Supplier::setId(int id) { this->id = id; }
    void Supplier::setName(const std::string &name) { this->name = name; }
    void Supplier::setAddress(const std::string &address) { this->address = address; }
    void Supplier::setPhone(const std::string &phone) { this->phone = phone; }
    void Supplier::setEmail(const std::string &email) { this->email = email; }

    // Serialization to JSON
    //  Serialization to JSON
    nlohmann::json Supplier::toJson() const {
        return {
            {"id", id},
            {"name", name},
            {"address", address},
            {"phone", phone},
            {"email", email}
        };
    }

    // Deserialization from JSON
    Supplier Supplier::fromJson(const nlohmann::json& j) {
        Supplier s;
        
        // Usamos .value("key", default) por seguridad.
        // Si el JSON no trae "name", pondrá "Unknown" en lugar de crashear.
        s.setId(j.value("id", 0));
        s.setName(j.value("name", "Unknown"));
        s.setAddress(j.value("address", ""));
        s.setPhone(j.value("phone", ""));
        s.setEmail(j.value("email", ""));

        return s;
    }        
} // Cerramos el namespace models