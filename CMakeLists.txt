cmake_minimum_required(VERSION 3.20)

project(MiBackendMonarca CXX)

# 1. Usar C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Modo Debug por defecto si no se especifica
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

message(STATUS "🔨 Modo de compilación: ${CMAKE_BUILD_TYPE}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(MONARCA_DEV)
    add_compile_definitions(DROGON_DEBUG_MODE)
endif()

# 3. Opciones de compilación
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 4. Buscar paquetes (vcpkg los resuelve)
find_package(Drogon    CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SQLiteCpp CONFIG REQUIRED)
find_package(cpr       CONFIG REQUIRED)
find_package(fmt       CONFIG REQUIRED)
find_package(jwt-cpp   CONFIG REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(OpenSSL   REQUIRED)

# 5. Definir el ejecutable
add_executable(MiBackendMonarca
    main.cc
    controllers/AuthController.h
    controllers/AuthController.cpp
    controllers/CommercialController.h
    controllers/CommercialController.cpp
    controllers/ProjectController.h
    controllers/ProjectController.cpp
    controllers/ProductCatalogController.h
    controllers/ProductCatalogController.cpp
    controllers/EmployeeController.h
    controllers/EmployeeController.cpp
    controllers/RolesController.h
    controllers/RolesController.cpp
    controllers/FinanceController.h
    controllers/FinanceController.cpp
    controllers/SupplierController.h
    controllers/SupplierController.cpp
    controllers/RawMaterialControllers/MetalMaterialController.h
    controllers/RawMaterialControllers/MetalMaterialController.cc
    models/Client.cc
    models/Supplier.h
    models/Supplier.cpp
    models/User.h
    models/User.cpp
    models/commercial/Client.h
    models/commercial/Client.cpp
    models/commercial/Project.h
    models/commercial/Project.cpp
    models/commercial/ProductCatalog.h
    models/commercial/ProductCatalog.cpp
    models/commercial/ProjectProduct.h
    models/commercial/ProjectProduct.cpp
    models/rrhh/Employee.h
    models/rrhh/Employee.cpp
    models/rrhh/Role.h
    models/rrhh/Role.cpp
    models/rrhh/EmployeePosition.h
    models/rrhh/EmployeePosition.cpp
    models/rrhh/EmployeeDependent.h
    models/rrhh/EmployeeDependent.cpp
    models/rrhh/EmployeeAbsence.h
    models/rrhh/EmployeeAbsence.cpp
    models/rrhh/EmployeeVacation.h
    models/rrhh/EmployeeVacation.cpp
    models/rrhh/EmployeeDisciplinary.h
    models/rrhh/EmployeeDisciplinary.cpp
    models/rawMaterial/MetalMaterial.h
    models/rawMaterial/MetalMaterial.cc
    utils/Database.h
    utils/Database.cpp
    utils/PgConnection.h
    utils/PgConnection.cpp
    utils/CryptoUtils.h
    utils/CryptoUtils.cpp
    utils/JwtUtils.h
    utils/JwtUtils.cpp
    utils/Adress.h
    utils/Adress.cc
    utils/FirebaseClient.h
    utils/FirebaseClient.cpp
)

# 6. Enlazar librerías
target_link_libraries(MiBackendMonarca PRIVATE
    Drogon::Drogon
    nlohmann_json::nlohmann_json
    SQLiteCpp
    cpr::cpr
    fmt::fmt
    jwt-cpp::jwt-cpp
    PostgreSQL::PostgreSQL
    OpenSSL::SSL
    OpenSSL::Crypto
)
