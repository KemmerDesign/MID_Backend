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

# En Debug activamos la bandera MONARCA_DEV para logs detallados
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

# 5. Definir el ejecutable
add_executable(MiBackendMonarca
    main.cc
    controllers/FinanceController.h
    controllers/FinanceController.cpp
    controllers/SupplierController.h
    controllers/SupplierController.cpp
    controllers/RawMaterialControllers/MetalMaterialController.h
    controllers/RawMaterialControllers/MetalMaterialController.cc
    models/Client.cc
    models/Supplier.h
    models/Supplier.cpp
    models/rawMaterial/MetalMaterial.h
    models/rawMaterial/MetalMaterial.cc
    utils/Database.h
    utils/Database.cpp
    utils/Adress.h
    utils/Adress.cc
)

# 6. Enlazar librerías
target_link_libraries(MiBackendMonarca PRIVATE
    Drogon::Drogon
    nlohmann_json::nlohmann_json
    SQLiteCpp
    cpr::cpr
    fmt::fmt
)
