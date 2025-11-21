cmake_minimum_required(VERSION 3.20)

project(FloodSim
        VERSION 1.0
        LANGUAGES CXX)

# ---------------------------------------------------------
# Configuración general del proyecto
# ---------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Multi-config (Visual Studio / Ninja Multi-Config)
if(NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------
# Configuración de vcpkg (si no usas el triplete por defecto)
# ---------------------------------------------------------
# set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")

# ---------------------------------------------------------
# Dependencias externas vía vcpkg
# ---------------------------------------------------------
find_package(GDAL CONFIG REQUIRED)
find_package(Boost REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)
find_package(nana CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)

# MQTT (solo si lo usas)
find_package(paho-mqttpp3 CONFIG QUIET)
find_package(paho-mqtt3as CONFIG QUIET)

# ---------------------------------------------------------
# Rutas
# ---------------------------------------------------------
include_directories(${CMAKE_SOURCE_DIR}/include)

# ---------------------------------------------------------
# Subdirectorios por módulos
# ---------------------------------------------------------
add_subdirectory(src/core)
add_subdirectory(src/io/gis)
add_subdirectory(src/io/mqtt)
add_subdirectory(src/io/taste)
add_subdirectory(src/gui)
add_subdirectory(src/main)

# ---------------------------------------------------------
# Opciones avanzadas (Testing, Benchmarks, Sanitizers)
# ---------------------------------------------------------
option(FLOODSIM_BUILD_TESTS "Build tests" OFF)
if(FLOODSIM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()