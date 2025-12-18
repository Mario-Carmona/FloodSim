
add_library(danasim_core
    CoreEngineCA.cpp
)

target_include_directories(danasim_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_core PUBLIC 
    danasim_options
    DanaSim::Common
    DanaSim::IO::GIS
    tensorflow_c
)

add_library(DanaSim::Core ALIAS danasim_core)
