
# =========================================================
# ⚙️ GLOBAL TARGET OPTIONS DEFINITION (INTERFACE TARGET)
# =========================================================

# Dependency Resolution
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(unofficial-whereami CONFIG REQUIRED)

# Force CMake to locate and utilize the LLVM-based OpenMP runtime implementation under MSVC
if(MSVC)
    set(OpenMP_CXX_FLAGS "/openmp:llvm")
    set(OpenMP_C_FLAGS "/openmp:llvm")
endif()

find_package(OpenMP REQUIRED)

# Define an interface configuration target to propagate global build policies
add_library(floodsim_options INTERFACE)

# 1. Compilation Directives & Feature Requirements
target_compile_features(floodsim_options INTERFACE cxx_std_20)

target_sources(floodsim_options
    INTERFACE
        logging/Logger.cpp
)

target_include_directories(floodsim_options
    INTERFACE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(floodsim_options 
    INTERFACE 
        fmt::fmt
        spdlog::spdlog_header_only
        unofficial::whereami::whereami
        OpenMP::OpenMP_CXX
)

if(WIN32)
    target_compile_definitions(floodsim_options INTERFACE 
        NOMINMAX
        WIN32_LEAN_AND_MEAN # Optimization: Accelerate compilation phases on Windows platforms
    )

    target_compile_options(floodsim_options INTERFACE
        # DEBUG CONFIGURATION
        $<$<CONFIG:Debug>:/Od /Zi /RTC1 /W4 /utf-8>
        # RELEASE CONFIGURATION
        $<$<CONFIG:Release>:/O2 /DNDEBUG /GL /W4 /utf-8>
        # RELWITHDEBINFO CONFIGURATION
        $<$<CONFIG:RelWithDebInfo>:/O2 /Zi /Zo /DNDEBUG /GL /W4 /utf-8>
    )
    
    # 2. Linker Directives (Typically required by MSVC for Link-Time Code Generation / LTCG)
    target_link_options(floodsim_options INTERFACE
        # DEBUG CONFIGURATION
        $<$<CONFIG:Debug>:/DEBUG>
        # RELEASE CONFIGURATION
        $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
        # RELWITHDEBINFO CONFIGURATION
        $<$<CONFIG:RelWithDebInfo>:/DEBUG /LTCG /OPT:REF /OPT:ICF /INCREMENTAL:NO>
    )
else()
    # GCC / Clang Toolchains
    target_compile_options(floodsim_options INTERFACE
        $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
        $<$<CONFIG:RelWithDebInfo>:-O3 -g -DNDEBUG>
    )
endif()

# ---------------------------------------------------------
# 📂 Subprojects & Target Hierarchy Traversals
# ---------------------------------------------------------
if(BUILD_APP OR BUILD_TESTS)
    add_subdirectory(app)
endif()

if(BUILD_APP)
    # 1. Always include the CLI target (serves as the core simulator foundation)
    add_subdirectory(cli)

    # 2. Conditionally append the Graphical User Interface (GUI) target layer
    if(ENABLE_GUI)
        add_subdirectory(gui)
    endif()
endif()
