
# =========================================================
# ⚙️ GLOBAL TARGET OPTIONS DEFINITION (INTERFACE TARGET)
# =========================================================

# Dependency Resolution
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(unofficial-whereami CONFIG REQUIRED)

# ==============================================================================
# OpenMP LLVM Runtime Configuration and Deployment
# ==============================================================================
# Force CMake to locate and utilize the LLVM-based OpenMP runtime implementation under MSVC
if(MSVC)    
    set(OpenMP_CXX_FLAGS "/openmp:llvm")    
    set(OpenMP_C_FLAGS "/openmp:llvm")
endif()

find_package(OpenMP REQUIRED)

if(WIN32)
    # 1. Attempt to dynamically detect the Visual Studio Tools directory.    
    get_filename_component(MSVC_COMPILER_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

    # Navigate up three levels in the directory tree to reach the base MSVC tools root    
    get_filename_component(MSVC_TOOLS_BASE_DIR "${MSVC_COMPILER_BIN_DIR}/../../../" ABSOLUTE)

    # 2. Extract the numeric version pattern of the toolset (e.g., 14.51.36231) using a regular expression    
    if(MSVC_TOOLS_BASE_DIR MATCHES "([0-9]+\\.[0-9]+\\.[0-9]+)")
        set(CUSTOM_MSVC_REDIST_VERSION "${CMAKE_MATCH_1}")        
        message(STATUS "Extracted MSVC Toolset Version: ${CUSTOM_MSVC_REDIST_VERSION}")    
    else()        
        message(WARNING "Failed to extract the MSVC Toolset version from the compiler path.")    
    endif()

    # 3. Construct the absolute path to the associated MSVC Redistributables folder
    get_filename_component(MSVC_REDIST_DIR "${MSVC_TOOLS_BASE_DIR}/../../../Redist/MSVC/${CUSTOM_MSVC_REDIST_VERSION}" ABSOLUTE)

    # 4. Recursively locate the LLVM OpenMP runtime DLL within the redistributable folder
    # Note: GLOB_RECURSE returns a list of files matching the pattern    
    file(GLOB_RECURSE OPENMP_LLVM_DLL_FILES "${MSVC_REDIST_DIR}/*libomp140.x86_64.dll")

    # 5. Install the located DLL(s) alongside the application binaries    
    install(FILES 
        ${OPENMP_LLVM_DLL_FILES}
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

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
