
add_library(danasim_ui_gui
    GUI_Interface.cpp
)

target_include_directories(danasim_ui_gui PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_ui_gui PUBLIC
    danasim_options
    DanaSim::UI::Base
)

add_library(DanaSim::UI::GUI ALIAS danasim_ui_gui)
