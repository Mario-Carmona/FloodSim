
# =========================================================
# 💾 Input/Output (I/O) Module Configuration
# =========================================================

# ---------------------------------------------------------
# 📂 Internal Module Hierarchy Traversals
# ---------------------------------------------------------
# Register data ingestion and serialization submodules
add_subdirectory(readers)
add_subdirectory(writers)

# ---------------------------------------------------------
# 🏗️ Interface Target Declaration
# ---------------------------------------------------------
# Define a purely structural interface library.
# This target compiles no sources of its own, acting solely as an 
# umbrella component to aggregate the underlying I/O submodules.
add_library(floodsim_io INTERFACE)

# ---------------------------------------------------------
# 🔗 Linkage & Dependency Propagation
# ---------------------------------------------------------
# Consolidate readers and writers into the unified I/O interface,
# propagating their functionality to upstream consumers.
target_link_libraries(floodsim_io
    INTERFACE
        floodsim_io_readers
        floodsim_io_writers
)
