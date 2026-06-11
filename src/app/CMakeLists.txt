
# =========================================================
# 🚀 Application Core Module Configuration
# =========================================================

# ---------------------------------------------------------
# 📂 Internal Module Hierarchy Traversals
# ---------------------------------------------------------
# Register foundational input/output and configuration modules
if(BUILD_APP)
    add_subdirectory(io)
endif()

add_subdirectory(config)

# Conditionally inject the simulation core logic layer
if(ENABLE_CORE)
    add_subdirectory(core)
endif()

# Register architectural adapters for external system integration
add_subdirectory(adapters)

# ---------------------------------------------------------
# 🏗️ Core Library Component Declaration
# ---------------------------------------------------------
# Define the primary application library that orchestrates the underlying modules
add_library(floodsim_app)

# ---------------------------------------------------------
# 🎯 Source Compilation Units Registration
# ---------------------------------------------------------
target_sources(floodsim_app
    PRIVATE
        Application.cpp
)

# ---------------------------------------------------------
# 📂 Header Search Directories
# ---------------------------------------------------------
# Expose the global project include root to all downstream dependents
target_include_directories(floodsim_app
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# ---------------------------------------------------------
# 🔗 Linkage & Dependency Propagation
# ---------------------------------------------------------
# Consolidate internal submodules and global build options into the application layer
target_link_libraries(floodsim_app
    PUBLIC
        floodsim_options
        floodsim_config
        floodsim_core
        floodsim_adapters
)
