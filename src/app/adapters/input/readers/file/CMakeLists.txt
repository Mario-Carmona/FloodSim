

add_library(danasim_readers_file)

target_sources(danasim_readers_file
    PRIVATE
        static/FileStaticReader.cpp
        static/FileStaticReaderFactory.cpp
        static/IdrisiReader.cpp
        dynamic/FileDynamicReader.cpp
        dynamic/FileDynamicReaderFactory.cpp
        dynamic/HifReader.cpp
)

target_include_directories(danasim_readers_file
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_readers_file
    PUBLIC
        danasim_options
)
