
find_package(OpenGL REQUIRED)

find_package(imgui CONFIG REQUIRED)

find_package(glfw3 CONFIG REQUIRED)


find_path(PORTABLE_FILE_DIALOGS_INCLUDE_DIRS "portable-file-dialogs.h")



add_subdirectory(helpers)

add_subdirectory(tabs)



add_executable(danasim_gui
    mainGUI.cpp
)

target_link_libraries(danasim_gui
    PRIVATE
        danasim_options
        danasim_app
        danasim_gui_helpers
        danasim_gui_tabs
        imgui::imgui
        glfw
        OpenGL::GL
)

target_include_directories(danasim_gui 
    PRIVATE 
        ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
)

set_target_properties(danasim_gui PROPERTIES
    OUTPUT_NAME DanaSimGUI
    DEBUG_POSTFIX "d"
)
