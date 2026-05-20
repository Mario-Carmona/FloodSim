
add_subdirectory(io)

add_subdirectory(config)

if(ENABLE_CORE)
    add_subdirectory(core)
endif()

add_subdirectory(adapters)



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
        danasim_config
        danasim_core
        danasim_adapters
)
