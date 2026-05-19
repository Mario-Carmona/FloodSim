
add_subdirectory(file)


add_library(danasim_io_writers INTERFACE)

target_link_libraries(danasim_io_writers
    INTERFACE
        danasim_io_writers_file
)
