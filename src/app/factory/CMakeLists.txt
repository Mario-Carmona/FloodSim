
add_library(danasim_factories)

target_sources(danasim_factories
    PRIVATE
        InputFactory.cpp
        OutputFactory.cpp
        StateUpdaterFactory.cpp
)

target_include_directories(danasim_factories
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_factories
    PUBLIC
        danasim_options
        danasim_core
        danasim_adapters
)
