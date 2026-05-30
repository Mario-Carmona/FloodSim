
# =========================================================
# ⚙️ Application Configuration Module
# =========================================================

# ---------------------------------------------------------
# 📦 Dependency Resolution
# ---------------------------------------------------------
# Resolve the external YAML parsing and emission library
find_package(yaml-cpp REQUIRED)

# ---------------------------------------------------------
# 🏗️ Core Library Component Declaration
# ---------------------------------------------------------
# Define the library responsible for parsing, validating, and 
# managing application state configurations.
add_library(floodsim_config)

# ---------------------------------------------------------
# 🎯 Source Compilation Units Registration
# ---------------------------------------------------------
target_sources(floodsim_config
    PRIVATE
        ConfigLoader.cpp
)

# ---------------------------------------------------------
# 📂 Header Search Directories
# ---------------------------------------------------------
# Expose the global project include root to all downstream dependents
target_include_directories(floodsim_config
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# ---------------------------------------------------------
# 🔗 Linkage & Dependency Propagation
# ---------------------------------------------------------
# Inherit global build policies and inject the YAML parser dependency 
# to propagate configuration capabilities upstream.
target_link_libraries(floodsim_config
    PUBLIC
        floodsim_options
        yaml-cpp::yaml-cpp
)
