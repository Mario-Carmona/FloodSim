
cmake_minimum_required(VERSION 4.1.1)


project(
    DanaSim
    VERSION 1.0.0
    LANGUAGES CXX
)


# =========================================================
# Opciones globales
# =========================================================
option(DANASIM_BUILD_APPS "Build Apps" OFF)
option(DANASIM_BUILD_DOCS "Build Docs" OFF)
option(DANASIM_BUILD_GUI "Build GUI executable" OFF)
option(DANASIM_SILENT_DEFAULT "Silent logging by default" OFF)


message(STATUS "Build Configuration:")
message(STATUS "  - Build Apps: ${DANASIM_BUILD_APPS}")
message(STATUS "  - Build Docs: ${DANASIM_BUILD_DOCS}")


if(DANASIM_BUILD_APPS)
    add_subdirectory(src)
endif()

if(DANASIM_BUILD_DOCS)

endif()
