

add_subdirectory(helpers)



add_library(danasim_gui_tabs)

target_sources(danasim_gui_tabs
    PRIVATE
        MainControlTab.cpp
        InputTab.cpp
        LoggingTab.cpp
        OutputTab.cpp
        ScenarioTab.cpp
        SimulationTab.cpp
        StateUpdaterTab.cpp
)

target_include_directories(danasim_gui_tabs
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_gui_tabs
    PUBLIC
        danasim_options
        danasim_gui_tabs_helpers
        imgui::imgui
        glfw
        OpenGL::GL
)

target_include_directories(danasim_gui_tabs
    PRIVATE 
        ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
)
