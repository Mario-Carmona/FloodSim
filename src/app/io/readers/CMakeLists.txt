
# =========================================================
# 📥 Input/Output (I/O) Data Ingestion Submodule Configuration
# =========================================================

# ---------------------------------------------------------
# 📂 Internal Module Hierarchy Traversals
# ---------------------------------------------------------
# Register concrete file-based data ingestion engines and parsers
add_subdirectory(file)

# ---------------------------------------------------------
# 🏗️ Interface Target Declaration
# ---------------------------------------------------------
# Define a structural interface library for input operations.
# This target orchestrates underlying data reading implementations
# without compiling source files locally.
add_library(floodsim_io_readers INTERFACE)

# ---------------------------------------------------------
# 🔗 Linkage & Dependency Propagation
# ---------------------------------------------------------
# Consolidate specific ingestion components into the unified readers layer,
# mapping their configurations to upstream architectural components.
target_link_libraries(floodsim_io_readers
    INTERFACE
        floodsim_io_readers_file
)
