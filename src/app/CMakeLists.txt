
add_subdirectory(config)
add_subdirectory(factory)
add_subdirectory(logging)


add_library(danasim_app)

target_sources(danasim_app
    PRIVATE
        Application.cpp
)

target_include_directories(danasim_app
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_app
    PUBLIC
        danasim_options
        danasim_logging
        danasim_config
        danasim_factories
)
