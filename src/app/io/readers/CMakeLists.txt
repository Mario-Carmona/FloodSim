

add_subdirectory(file)


add_library(danasim_io_readers INTERFACE)

target_link_libraries(danasim_io_readers
    INTERFACE
        danasim_io_readers_file
)
