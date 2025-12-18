
cmake_minimum_required(VERSION 4.1.1)


project(DanaSim VERSION 1.0 LANGUAGES CXX)


message(STATUS "Build Configuration:")
message(STATUS "  - Build Apps: ${DANASIM_BUILD_APPS}")
message(STATUS "  - Build Docs: ${DANASIM_BUILD_DOCS}")



# =========================================================
# DEFINICIÓN DE OPCIONES GLOBALES (INTERFACE TARGET)
# =========================================================
add_library(danasim_options INTERFACE)

# 1. Opciones de COMPILACIÓN (Compile Flags)
target_compile_features(danasim_options INTERFACE cxx_std_20)

if(MSVC)
    target_compile_options(danasim_options INTERFACE
        # DEBUG
        $<$<CONFIG:Debug>:/Od /Zi /RTC1 /W4>
        # RELEASE
        $<$<CONFIG:Release>:/O2 /DNDEBUG /GL /W4>
    )
    
    # 2. Opciones de LINKER (Solo MSVC suele necesitar esto para LTCG)
    target_link_options(danasim_options INTERFACE
        $<$<CONFIG:Debug>:/DEBUG>
        $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
    )
else()
    # GCC / Clang
    target_compile_options(danasim_options INTERFACE
        $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
    )
endif()





if(DANASIM_BUILD_APPS)
    add_subdirectory(src)
endif()


# =========================================================
# CONFIGURACIÓN DE EMPAQUETADO (CPack)
# =========================================================

# 🎯 SOLUCIÓN: Carga el módulo de CPack para inicializar todas las variables
include(CPack) 

# (Opcional, pero recomendado para instaladores de Windows)
# Incluye las librerías de tiempo de ejecución del sistema necesarias (como los runtimes de MSVC)
# para que el instalador WIX/MSI las pueda incluir.
include(InstallRequiredSystemLibraries)

# =========================================================


if(DANASIM_BUILD_DOCS)

endif()
