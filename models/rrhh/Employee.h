#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "../../utils/PgConnection.h"

using json = nlohmann::json;

struct Employee {
    std::string id;
    std::string tenant_id;
    std::string user_id;           // nullable — cuenta de sistema vinculada
    std::string nombres;
    std::string apellidos;
    std::string tipo_documento;
    std::string numero_documento;
    std::string email_personal;
    std::string email_corporativo;
    std::string telefono1;
    std::string telefono2;
    std::string fecha_nacimiento;
    std::string direccion;
    std::string ciudad;
    std::string cargo;
    std::string departamento;
    std::string tipo_trabajador;  // empleado_directo | contratista
    std::string tipo_contrato;
    std::string fecha_ingreso;
    std::string fecha_retiro;
    std::string salario_base;
    // Campos exclusivos de contratistas
    std::string valor_honorarios;
    std::string periodicidad_pago;
    std::string objeto_contrato;
    std::string fecha_fin_contrato;
    bool        activo = true;
    std::string notas;
    std::string created_at;
    std::string updated_at;

    std::string fullName() const { return nombres + " " + apellidos; }
    json toJson() const;

    static Employee                create(const std::string& tenant_id, const json& data);
    static std::optional<Employee> findById(const std::string& id, const std::string& tenant_id);
    static std::vector<Employee>   findAll(const std::string& tenant_id,
                                           const std::string& departamento = "");
};
