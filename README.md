# Backend Service en C++ (Drogon + PostgreSQL + GCP)

Este proyecto es el motor de **Monarca I+D**, un backend de alto rendimiento escrito en **C++20** utilizando el framework **Drogon**. 

## 🚀 Visión Macro: SaaS ERP Multi-Tenant
Este backend está diseñado para soportar una arquitectura **SaaS Multi-Tenant (Multi-Inquilino)**, que se comercializará a múltiples empresas del sector metalúrgico.

- **Aislamiento de Datos:** Cada empresa operará en un entorno aislado. Se utilizará PostgreSQL con una estrategia de **Schemas por Tenant** o **Row-Level Security (tenant_id)** para garantizar la privacidad.
- **Módulos Dinámicos:** El sistema debe permitir habilitar/deshabilitar características (Finanzas, Stock, Producción, RRHH) según el plan contratado por la empresa.
- **Datos Transversales B2B:** El backend gestionará tablas globales compartidas, como una **Lista Maestra de Proveedores** con precios actualizados diariamente mediante web scraping, permitiendo a los inquilinos subcontratarse entre sí.
- **Despliegue:** Preparado para ser desplegado en **Google Cloud Run**, conectándose a Cloud SQL (PostgreSQL).

---

## 🛠️ Stack Tecnológico

* **Lenguaje:** C++20
* **Compilador:** Clang (Recomendado) o GCC.
* **Sistema de Construcción:** CMake (>3.20).
* **Gestor de Paquetes:** vcpkg (Modo Manifiesto).
* **Framework Web:** Drogon.
* **Base de Datos:** PostgreSQL (Principal) y SQLite (Caché local/testing).
* **Librerías Clave:**
    * `nlohmann-json`: Manipulación de JSON.
    * `jwt-cpp`: Autenticación y Autorización basada en Roles y Tenants.
    * `cpr`: Cliente HTTP para Web Scraping y APIs externas.
    * `fmt`: Formateo de texto moderno.

---

## 🚀 Guía de Instalación y Compilación

Sigue estos pasos para configurar el entorno desde cero en Linux (Ubuntu/WSL).

### 1. Prerrequisitos del Sistema
Instala las herramientas básicas de compilación y librerías del sistema necesarias para que `vcpkg` pueda compilar las dependencias.

```bash
sudo apt update
sudo apt install build-essential cmake git curl zip unzip tar pkg-config clang lld libpq-dev -y

# Instalación de VCPKG
cd ~
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh

# Configurar el build
cmake -B build -S . \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake

# Compilar
cmake --build build
```
