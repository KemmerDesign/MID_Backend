cmake_minimum_required(VERSION 3.20)

project(MiBackendMonarca CXX)

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

# 4. Definir el ejecutable
add_executable(MiBackendMonarca main.cc
    controllers/FinanceController.h
    controllers/FinanceController.cpp
    controllers/SupplierController.h
    controllers/SupplierController.cpp
    models/Supplier.h
    models/Supplier.cpp
)

# 5. Enlazar librerías
target_link_libraries(MiBackendMonarca PRIVATE
    Drogon::Drogon
    nlohmann_json::nlohmann_json
    jwt-cpp::jwt-cpp
    cpr::cpr
    fmt::fmt
)
