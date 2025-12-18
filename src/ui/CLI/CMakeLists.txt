
add_library(danasim_ui_cli
    CLI_Interface.cpp
)

target_include_directories(danasim_ui_cli PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_ui_cli PUBLIC
    danasim_options
    DanaSim::UI::Base
    DanaSim::IO::GIS
)

add_library(DanaSim::UI::CLI ALIAS danasim_ui_cli)
