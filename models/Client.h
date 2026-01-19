#pragma once // Evita inclusiones múltiples del archivo
#include "../utils/Adress.h"
#include <string>
#include <vector>
#include <iostream>
#include <nlohmann/json.hpp>

namespace models
{
    class Client
    {
    private:
        int id;
        std::string name;
        std::string comercial_name;
        std::string id_type;
        std::string id_number;
        std::string email;
        std::string phone;
        std::string type;
        bool active;
        double credit_limit;
        int credit_days;
        std::vector<utils::Adress> addresses;
    public:
        //Contructor vacio
        Client();

        //Metodos clave para interactuar con la API
        nlohmann::json toJson() const;
        static Client fromJson(const nlohmann::json& j);

        //Getters
        int getId() const;
        std::string getName() const;
        std::string getComercialName() const;
        std::string getIdType() const;
        std::string getIdNumber() const;
        std::string getEmail() const;
        std::string getPhone() const;
        std::string getType() const;
        bool getActive() const;
        double getCreditLimit() const;
        int getCreditDays() const;
        std::vector<utils::Adress> getAddresses() const;

        //Setters
        void setId(int id);
        void setName(const std::string& name);
        void setComercialName(const std::string& comercial_name);
        void setIdType(const std::string& id_type);
        void setIdNumber(const std::string& id_number);
        void setEmail(const std::string& email);
        void setPhone(const std::string& phone);
        void setType(const std::string& type);
        void setActive(bool active);
        void setCreditLimit(double credit_limit);
        void setCreditDays(int credit_days);
        void setAddresses(std::vector<utils::Adress> addresses);
    };

}