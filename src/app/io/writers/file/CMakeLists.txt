

add_library(danasim_io_writers_file)

target_sources(danasim_io_writers_file
    PRIVATE
        static/FileStaticWriter.cpp
        static/FileStaticWriterFactory.cpp
        static/IdrisiWriter.cpp
)

target_include_directories(danasim_io_writers_file
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_io_writers_file
    PUBLIC
        danasim_options
)
