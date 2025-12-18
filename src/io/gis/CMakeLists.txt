
add_library(danasim_io_gis
    GISDataHandler.cpp
)

target_include_directories(danasim_io_gis PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_io_gis PUBLIC
    danasim_options
    DanaSim::Common
    GDAL::GDAL
)

add_library(DanaSim::IO::GIS ALIAS danasim_io_gis)
