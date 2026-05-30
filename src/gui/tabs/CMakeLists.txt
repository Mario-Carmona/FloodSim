
# =========================================================
# 🗂️ GUI Tab Views Component Configuration
# =========================================================

# Traverse into subdirectories to resolve UI layout helper targets
add_subdirectory(helpers)

# Define the modular library bundling all user interface tab views
add_library(floodsim_gui_tabs)

# ---------------------------------------------------------
# 🎯 Source Compilation Units Registration
# ---------------------------------------------------------
target_sources(floodsim_gui_tabs
    PRIVATE
        MainControlTab.cpp
        InputTab.cpp
        LoggingTab.cpp
        OutputTab.cpp
        ScenarioTab.cpp
        SimulationTab.cpp
        StateUpdaterTab.cpp
)

# ---------------------------------------------------------
# 🔗 Linkage & Dependency Propagation
# ---------------------------------------------------------
target_link_libraries(floodsim_gui_tabs
    PUBLIC
        floodsim_options
        floodsim_gui_tabs_helpers
        imgui::imgui
        glfw
        OpenGL::GL
)

# ---------------------------------------------------------
# 📂 Header Search Directories
# ---------------------------------------------------------
# Expose the global project include root to all downstream dependents
target_include_directories(floodsim_gui_tabs
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# Inject private search paths restricted solely to local dependency translation units
target_include_directories(floodsim_gui_tabs
    PRIVATE 
        ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
)
