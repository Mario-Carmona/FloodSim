
# =========================================================
# 💾 Input/Output (I/O) Serialization Submodule Configuration
# =========================================================

# ---------------------------------------------------------
# 📂 Internal Module Hierarchy Traversals
# ---------------------------------------------------------
# Register concrete file-based serialization engines and handlers
add_subdirectory(file)

# ---------------------------------------------------------
# 🏗️ Interface Target Declaration
# ---------------------------------------------------------
# Define a structural interface library for output operations.
# This target orchestrates underlying data writing implementations
# without compiling source files locally.
add_library(floodsim_io_writers INTERFACE)

# ---------------------------------------------------------
# 🔗 Linkage & Dependency Propagation
# ---------------------------------------------------------
# Consolidate specific serialization components into the unified writers layer,
# mapping their configurations to upstream architectural components.
target_link_libraries(floodsim_io_writers
    INTERFACE
        floodsim_io_writers_file
)
