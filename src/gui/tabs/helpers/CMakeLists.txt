
add_library(danasim_gui_tabs_helpers)

target_sources(danasim_gui_tabs_helpers
    PRIVATE
        Helpers.cpp
)

target_include_directories(danasim_gui_tabs_helpers
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_gui_tabs_helpers
    PUBLIC
        danasim_options
        imgui::imgui
        glfw
        OpenGL::GL
)

target_include_directories(danasim_gui_tabs_helpers
    PRIVATE 
        ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
)
