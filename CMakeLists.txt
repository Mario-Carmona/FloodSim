
cmake_minimum_required(VERSION 3.25.0)


project(DanaSim VERSION 1.0 LANGUAGES CXX)


message(STATUS "Build Configuration:")
message(STATUS "  - Build Apps: ${BUILD_APPS}")
message(STATUS "  - Build Docs: ${BUILD_DOCS}")



# =========================================================
# DEFINICIÓN DE OPCIONES GLOBALES (INTERFACE TARGET)
# =========================================================
add_library(danasim_options INTERFACE)

# 1. Opciones de COMPILACIÓN (Compile Flags)
target_compile_features(danasim_options INTERFACE cxx_std_17)

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



# ---------------------------------------------------------
# Dependencias externas vía vcpkg
# ---------------------------------------------------------
find_package(tensorflow-cc CONFIG REQUIRED)





if(DANASIM_BUILD_APPS)
    add_subdirectory(src)
endif()



if(DANASIM_BUILD_DOCS)

endif()
