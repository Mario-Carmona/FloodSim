
add_library(danasim_gui_helpers)

target_sources(danasim_gui_helpers
    PRIVATE
        Helpers.cpp
)

target_include_directories(danasim_gui_helpers
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_gui_helpers
    PUBLIC
        danasim_options
)
