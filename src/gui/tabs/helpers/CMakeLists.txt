
# =========================================================
# 🛠️ GUI Tab Utilities & Helper Components
# =========================================================

# Define the utility helper library for GUI tab view operations
add_library(floodsim_gui_tabs_helpers)

# ---------------------------------------------------------
# 🎯 Source Compilation Units Registration
# ---------------------------------------------------------
target_sources(floodsim_gui_tabs_helpers
    PRIVATE
        Helpers.cpp
)

# ---------------------------------------------------------
# 🔗 Linkage & Dependency Propagation
# ---------------------------------------------------------
# Propagate UI framework dependencies to any target consuming these helpers
target_link_libraries(floodsim_gui_tabs_helpers
    PUBLIC
        floodsim_options
        imgui::imgui
        glfw
        OpenGL::GL
)

# ---------------------------------------------------------
# 📂 Header Search Directories
# ---------------------------------------------------------
# Expose the global project include root to all downstream dependents
target_include_directories(floodsim_gui_tabs_helpers
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# Inject private search paths restricted solely to local dependency translation units
target_include_directories(floodsim_gui_tabs_helpers
    PRIVATE 
        ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
)
