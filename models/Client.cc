#include "Client.h"



namespace models{
        Client::Client() = default;
        //Metodos clave para interactuar con la API
        nlohmann::json Client::toJson() const{
            // Serialize client data to JSON
            // La idea es crear un objeto JSON y llenar sus campos con los atributos del cliente.
            nlohmann::json j;
            j["id"] = id;
            j["name"] = name;
            j["comercial_name"] = comercial_name;
            j["id_type"] = id_type;
            j["id_number"] = id_number;
            j["email"] = email;
            j["phone"] = phone;
            j["type"] = type;
            j["active"] = active;
            j["credit_limit"] = credit_limit;
            j["credit_days"] = credit_days;

            // Serialize addresses
            j["addresses"] = nlohmann::json::array();
            for (const auto& address : addresses) {
                j["addresses"].push_back(address.toJson());
            }

            return j;
        }

        Client Client::fromJson(const nlohmann::json& j) {
            // Creamos un objeto de tipo de Client para poder construirlo desde un JSON
            Client c;
            c.id = j.value("id", 0);
            c.name = j.value("name", "");
            c.comercial_name = j.value("comercial_name", "");
            c.id_type = j.value("id_type", "");
            c.id_number = j.value("id_number", ""); 
            c.email = j.value("email", "");
            c.phone = j.value("phone", "");
            c.type = j.value("type", "");
            c.active = j.value("active", false);
            c.credit_limit = j.value("credit_limit", 0.0);
            c.credit_days = j.value("credit_days", 0);

            //validamos con un if si el json tiene direcciones y adicional si es de tipo array.

            if(j.contains("addresses") && j["addresses"].is_array()) {
                for (const auto& addr_json : j["addresses"]) {
                    c.addresses.push_back(utils::Adress::fromJson(addr_json));
                }
            }
            return c;
        }

        //Getters
        int Client::getId() const{return id;};
        std::string Client::getName() const{return name;};
        std::string Client::getComercialName() const{return comercial_name;};
        std::string Client::getIdType() const{return id_type;};
        std::string Client::getIdNumber() const{return id_number;};
        std::string Client::getEmail() const {return email;};
        std::string Client::getPhone() const {return phone;};
        std::string Client::getType() const{return type;};
        bool Client::getActive() const{return active;};
        double Client::getCreditLimit() const {return credit_limit;};
        int Client::getCreditDays() const {return credit_days;};
        std::vector<utils::Adress> Client::getAddresses() const {return addresses;};

        //Setters
        void Client::setId(int id) {this->id = id;};
        void Client::setName(const std::string& name) {this -> name = name;};
        void Client::setComercialName(const std::string& comercial_name) {this -> comercial_name = comercial_name;};
        void Client::setIdType(const std::string& id_type) {this->id_type = id_type;};
        void Client::setIdNumber(const std::string& id_number) {this->id_number = id_number;};
        void Client::setEmail(const std::string& email) {this->email = email;}
        void Client::setPhone(const std::string& phone) {this -> phone = phone;};
        void Client::setType(const std::string& type) {this -> type = type;};
        void Client::setActive(bool active) {this -> active = active;};
        void Client::setCreditLimit(double credit_limit) {this -> credit_limit = credit_limit;};
        void Client::setCreditDays(int credit_days) {this -> credit_days = credit_days;};
        void Client::setAddresses(std::vector<utils::Adress> addresses) {
            // std::move convierte 'addresses' en un r-value, permitiendo 
            // que el vector de la clase 'robe' los datos sin copiar memoria.
            this->addresses = std::move(addresses);
        }
}