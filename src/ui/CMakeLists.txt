
add_library(danasim_ui_base
    UserInterface.cpp
)

target_include_directories(danasim_ui_base PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_ui_base PUBLIC
    danasim_options
    DanaSim::Core 
    DanaSim::Common
)

add_library(DanaSim::UI::Base ALIAS danasim_ui_base)


add_subdirectory(CLI)
add_subdirectory(GUI)
