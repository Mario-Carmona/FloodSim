
# =========================================================
# DEFINICIÓN DE OPCIONES GLOBALES (INTERFACE TARGET)
# =========================================================

# fmt
find_package(fmt CONFIG REQUIRED)

find_package(spdlog CONFIG REQUIRED)

find_package(unofficial-whereami CONFIG REQUIRED)

find_package(OpenMP REQUIRED)



add_library(danasim_options INTERFACE)

# 1. Opciones de COMPILACIÓN (Compile Flags)
target_compile_features(danasim_options INTERFACE cxx_std_20)

target_sources(danasim_options
    INTERFACE
        logging/Logger.cpp
)

target_include_directories(danasim_options
    INTERFACE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_options 
    INTERFACE 
        fmt::fmt
        spdlog::spdlog_header_only
        unofficial::whereami::whereami
        OpenMP::OpenMP_CXX
)

if(WIN32)
    target_compile_definitions(danasim_options INTERFACE 
        NOMINMAX
        WIN32_LEAN_AND_MEAN # Opcional: Acelera la compilación en Windows
    )

    target_compile_options(danasim_options INTERFACE
        # DEBUG
        $<$<CONFIG:Debug>:/Od /Zi /RTC1 /W4 /utf-8 /openmp:llvm>
        # RELEASE
        $<$<CONFIG:Release>:/O2 /DNDEBUG /GL /W4 /utf-8 /openmp:llvm>
        # RELWITHDEBINFO
        $<$<CONFIG:RelWithDebInfo>:/O2 /Zi /Zo /DNDEBUG /GL /W4 /utf-8 /openmp:llvm>
    )
    
    # 2. Opciones de LINKER (Solo MSVC suele necesitar esto para LTCG)
    target_link_options(danasim_options INTERFACE
        # DEBUG
        $<$<CONFIG:Debug>:/DEBUG>
        # RELEASE
        $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
        # RELWITHDEBINFO
        $<$<CONFIG:RelWithDebInfo>:/DEBUG /LTCG /OPT:REF /OPT:ICF /INCREMENTAL:NO>
    )
else()
    # GCC / Clang
    target_compile_options(danasim_options INTERFACE
        $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
        $<$<CONFIG:RelWithDebInfo>:-O3 -g -DNDEBUG>
    )
endif()





# -----------------------
# Subproyectos
# -----------------------
if(ENABLE_CORE)
    add_subdirectory(core)
endif()

add_subdirectory(adapters)

if(ENABLE_APP)
    add_subdirectory(app)
endif()

if(BUILD_APP)
    if(ENABLE_GUI)
        add_subdirectory(gui)
    else()
        add_subdirectory(cli)
    endif()
endif()



# =========================================================
# CONFIGURACIÓN DE EMPAQUETADO (CPack)
# =========================================================

# SOLUCIÓN: Carga el módulo de CPack para inicializar todas las variables
include(CPack) 

# (Opcional, pero recomendado para instaladores de Windows)
# Incluye las librerías de tiempo de ejecución del sistema necesarias (como los runtimes de MSVC)
# para que el instalador WIX/MSI las pueda incluir.
include(InstallRequiredSystemLibraries)

# =========================================================
