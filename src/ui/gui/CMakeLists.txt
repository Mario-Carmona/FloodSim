add_library(flood_gui
    GuiApp.cpp
)

target_include_directories(flood_gui PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(flood_gui
    PRIVATE nana
            fmt::fmt
            flood_core
)