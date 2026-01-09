# Backend Service en C++ (Drogon + Google Cloud)

Este proyecto es un backend de alto rendimiento escrito en **C++20** utilizando el framework **Drogon**. Está diseñado para ser desplegado en **Google Cloud Run** y consumir servicios de Google (Firebase, Sheets, Vertex AI) mediante APIs REST.

El objetivo es demostrar la viabilidad de C++ moderno para desarrollo web en entornos serverless, manteniendo una huella de memoria mínima.

## 🛠️ Stack Tecnológico

* **Lenguaje:** C++20
* **Compilador:** Clang (Recomendado) o GCC.
* **Sistema de Construcción:** CMake (>3.20).
* **Gestor de Paquetes:** vcpkg (Modo Manifiesto).
* **Framework Web:** Drogon.
* **Librerías Clave:**
    * `nlohmann-json`: Manipulación de JSON.
    * `jwt-cpp`: Creación de tokens para Auth de Google.
    * `cpr`: Cliente HTTP (Curl wrapper).
    * `fmt`: Formateo de texto moderno.

---

## 🚀 Guía de Instalación y Compilación

Sigue estos pasos para configurar el entorno desde cero en Linux (Ubuntu/WSL).

### 1. Prerrequisitos del Sistema
Instala las herramientas básicas de compilación y librerías del sistema necesarias para que `vcpkg` pueda compilar las dependencias.

```bash
sudo apt update
sudo apt install build-essential cmake git curl zip unzip tar pkg-config clang lld -y

instalacion de VCPKG

# 1. Clonar el repositorio en tu carpeta de usuario
cd ~
git clone [https://github.com/microsoft/vcpkg.git](https://github.com/microsoft/vcpkg.git)

# 2. Compilar el ejecutable de vcpkg
cd vcpkg
./bootstrap-vcpkg.sh

# 3. (Opcional) Agregar al PATH para usar el comando 'vcpkg' desde cualquier lado
# Agrega esta línea a tu .bashrc o .zshrc:
# export PATH=$PATH:~/vcpkg

# Crear carpeta de construcción y configurar
# Asumimos que vcpkg está en ~/vcpkg. Si está en otro lado, ajusta la ruta.

cmake -B build -S . \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake

cmake --build build