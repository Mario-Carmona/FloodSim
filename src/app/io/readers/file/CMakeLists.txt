
# =========================================================
# 📄 File-Based Data Ingestion Engine Configuration
# =========================================================

# Define the concrete library component responsible for file-based data input
add_library(floodsim_io_readers_file)

# ---------------------------------------------------------
# 🎯 Source Compilation Units Registration
# ---------------------------------------------------------
# Register the private implementation files for static reading operations (e.g., Idrisi format)
# as well as dynamic temporal data stream parsers (e.g., HIF format).
target_sources(floodsim_io_readers_file
    PRIVATE
        # Static data parsers
        static/FileStaticReader.cpp
        static/FileStaticReaderFactory.cpp
        static/IdrisiReader.cpp
        
        # Dynamic data stream handlers
        dynamic/FileDynamicReader.cpp
        dynamic/FileDynamicReaderFactory.cpp
        dynamic/HifReader.cpp
)

# ---------------------------------------------------------
# 📂 Header Search Directories
# ---------------------------------------------------------
# Expose the global project include root to all downstream dependents
target_include_directories(floodsim_io_readers_file
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# ---------------------------------------------------------
# 🔗 Linkage & Dependency Propagation
# ---------------------------------------------------------
# Inherit global build policies and compiler directives
target_link_libraries(floodsim_io_readers_file
    PUBLIC
        floodsim_options
)
