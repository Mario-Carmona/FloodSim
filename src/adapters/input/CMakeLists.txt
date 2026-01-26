
add_library(danasim_input_adapters)

target_sources(danasim_input_adapters
    PRIVATE
        FileInput.cpp
)

target_include_directories(danasim_input_adapters
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_input_adapters
    PUBLIC
        danasim_options
        danasim_core
        GDAL::GDAL
)
