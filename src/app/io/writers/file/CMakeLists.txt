
# =========================================================
# 📄 File-Based Serialization Engine Configuration
# =========================================================

# Define the concrete library component responsible for file-based data output
add_library(floodsim_io_writers_file)

# ---------------------------------------------------------
# 🎯 Source Compilation Units Registration
# ---------------------------------------------------------
# Register the private implementation files for static writing operations
# and specific file format handlers (e.g., Idrisi)
target_sources(floodsim_io_writers_file
    PRIVATE
        static/FileStaticWriter.cpp
        static/FileStaticWriterFactory.cpp
        static/IdrisiWriter.cpp
)

# ---------------------------------------------------------
# 📂 Header Search Directories
# ---------------------------------------------------------
# Expose the global project include root to all downstream dependents
target_include_directories(floodsim_io_writers_file
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# ---------------------------------------------------------
# 🔗 Linkage & Dependency Propagation
# ---------------------------------------------------------
# Inherit global build policies and compiler directives
target_link_libraries(floodsim_io_writers_file
    PUBLIC
        floodsim_options
)
