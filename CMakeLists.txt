cmake_minimum_required(VERSION 3.20)

project(MiBackendMonarca CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug") # Por defecto será Debug (Desarrollo)
endif()

message(STATUS "🔨 MODO ACTUAL: ${CMAKE_BUILD_TYPE}")

# Si estamos en Debug, activamos la bandera MONARCA_DEV
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(MONARCA_DEV)
endif()

# 1. Usar C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Opciones de compilación (Banderas recomendadas)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 3. Buscar paquetes (vcpkg hace la magia aquí)
find_package(Drogon CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_path(PICOJSON_INCLUDE_DIRS "picojson/picojson.h")

# 4. Configuración de build types
# Configuracion de entornos de compilacion
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug") # Por defecto será Debug si no especificamos nada
endif()

message(STATUS "🔨 Modo de compilación: ${CMAKE_BUILD_TYPE}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(MONARCA_DEV)
    # También le decimos a Drogon que muestre logs detallados
    add_compile_definitions(DROGON_DEBUG_MODE) 
endif()

# 5. Definir el ejecutable
add_executable(MiBackendMonarca main.cc
    controllers/FinanceController.h
    controllers/FinanceController.cpp
    controllers/SupplierController.h
    controllers/SupplierController.cpp
    models/Supplier.h
    models/Supplier.cpp
    utils/FirebaseClient.h
    utils/FirebaseClient.cpp
)

target_compile_definitions(MiBackendMonarca PRIVATE JWT_DISABLE_PICOJSON)

# 6. Enlazar librerías
target_link_libraries(MiBackendMonarca PRIVATE
    Drogon::Drogon
    nlohmann_json::nlohmann_json
    jwt-cpp::jwt-cpp
    cpr::cpr
    fmt::fmt
    OpenSSL::SSL 
    OpenSSL::Crypto
)
