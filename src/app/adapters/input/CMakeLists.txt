
# =========================================================
# 🔌 Data Ingestion (Input) Adapters Configuration
# =========================================================

# Define the concrete adapter library responsible for bridging external 
# data sources with the core simulation domain.
add_library(floodsim_input_adapters)

# ---------------------------------------------------------
# 🎯 Source Compilation Units Registration
# ---------------------------------------------------------
# Register private implementations of driving adapters handling file-system ingestion
target_sources(floodsim_input_adapters
    PRIVATE
        FileInput.cpp
)

# ---------------------------------------------------------
# 📂 Header Search Directories
# ---------------------------------------------------------
# Expose the global project include root to all downstream dependents
target_include_directories(floodsim_input_adapters
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# ---------------------------------------------------------
# 🔗 Linkage & Dependency Propagation
# ---------------------------------------------------------
# Bind the core logic layers and input readers to the adapter boundary,
# propagating capabilities to upstream architectural layers.
target_link_libraries(floodsim_input_adapters
    PUBLIC
        floodsim_options
        floodsim_core
        floodsim_io_readers
)
