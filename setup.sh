#!/bin/bash

echo "Iniciando configuración inicial..."

# Actualizar lista de paquetes
sudo apt update

# Instalar herramientas esenciales (compilador, make, cmake, git)
sudo apt install -y build-essential ninja-build curl zip unzip tar g++-12 gcc-12

# 1. Descargar el instalador oficial (.sh)
wget https://github.com/Kitware/CMake/releases/download/v4.1.1/cmake-4.1.1-linux-x86_64.sh

# 2. Crear carpeta de instalación y dar permisos
sudo chmod +x cmake-4.1.1-linux-x86_64.sh
sudo mkdir -p /opt/cmake

# 3. Ejecutar el script de instalación
# --skip-license: Acepta la licencia automáticamente
# --prefix: Dice dónde instalarlo
sudo ./cmake-4.1.1-linux-x86_64.sh --skip-license --prefix=/opt/cmake

rm ./cmake-4.1.1-linux-x86_64.sh

# 4. Crear el enlace simbólico para poder ejecutarlo desde cualquier lado
sudo ln -sf /opt/cmake/bin/cmake /usr/local/bin/cmake


sudo apt install -y curl zip unzip tar

./external/vcpkg/bootstrap-vcpkg.sh



# zlib
sudo apt install -y pkg-config

# openssl
sudo apt install -y linux-libc-dev


echo "Entorno listo. Ahora puedes ejecutar 'cmake ..'"