
# =========================================================
# 🧠 Core Simulation Logic Module Configuration
# =========================================================

# ---------------------------------------------------------
# 📦 Dependency Resolution
# ---------------------------------------------------------
# Resolve compile-time reflection framework for enumerations
find_package(magic_enum CONFIG REQUIRED)

# Resolve the geospatial library for coordinate transformations and spatial computations
find_package(GeographicLib CONFIG REQUIRED)

# ---------------------------------------------------------
# 🏗️ Core Library Component Declaration
# ---------------------------------------------------------
# Define the central domain logic library encapsulating the simulation 
# engine, spatial grids, and internal state management.
add_library(floodsim_core)

# ---------------------------------------------------------
# 🎯 Source Compilation Units Registration
# ---------------------------------------------------------
target_sources(floodsim_core
    PRIVATE
        # Primary simulation orchestrator
        SimulationCore.cpp
        
        # State checkpointing and memento pattern handlers
        snapshot/SnapshotManager.cpp
        snapshot/Snapshot.cpp
        
        # Spatial topology and geospatial mapping engines
        grid/MapGrid.cpp
)

# ---------------------------------------------------------
# 📂 Header Search Directories
# ---------------------------------------------------------
# Expose the global project include root to all downstream dependents
target_include_directories(floodsim_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# ---------------------------------------------------------
# 🔗 Linkage & Dependency Propagation
# ---------------------------------------------------------
# Inherit global build policies and inject geospatial/reflection engines 
# to propagate simulation domain capabilities upstream.
target_link_libraries(floodsim_core 
    PUBLIC 
        floodsim_options
        magic_enum::magic_enum
        ${GeographicLib_LIBRARIES}
)
