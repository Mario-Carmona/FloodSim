
add_subdirectory(readers)

add_subdirectory(writers)


add_library(danasim_io INTERFACE)

target_link_libraries(danasim_io
    INTERFACE
        danasim_io_readers
        danasim_io_writers
)
