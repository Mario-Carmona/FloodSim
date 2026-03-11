

add_subdirectory(file)

add_library(danasim_readers INTERFACE)

target_link_libraries(danasim_readers
    INTERFACE
        danasim_readers_file
)
