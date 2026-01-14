#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <nlohmann/json.hpp>

namespace models{
    class Supplier {

    private:
        int id;
        std::string name;
        std::string address;
        std::string phone;
        std::string email;

    public:
        Supplier();
        Supplier(int id, std::string name, std::string address, std::string phone, std::string email);
        // Getters
        int getId() const ;
        std::string getName() const ;
        std::string getAddress() const ;
        std::string getPhone() const ; 
        std::string getEmail() const ;

        // Setters
        void setId(int id);
        void setName(const std::string& name);
        void setAddress(const std::string& address);
        void setPhone(const std::string& phone);
        void setEmail(const std::string& email);

        // Serialization to JSON
        nlohmann::json toJson() const;

        // Deserialization from JSON
        static Supplier fromJson(const nlohmann::json& j);
        // ... (dentro de la clase FirebaseClient, sección public)



    };
}