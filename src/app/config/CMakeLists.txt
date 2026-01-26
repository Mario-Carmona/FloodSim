
add_library(danasim_config)

target_sources(danasim_config
    PRIVATE
        ConfigLoader.cpp
)

target_include_directories(danasim_config
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_config
    PUBLIC
        danasim_options
        yaml-cpp::yaml-cpp
)
