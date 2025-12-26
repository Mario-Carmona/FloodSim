
add_library(danasim_logging)

target_sources(danasim_logging
    PRIVATE
        Logger.cpp
)

target_include_directories(danasim_logging
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_logging
    PUBLIC
        danasim_options
        spdlog::spdlog_header_only
)
