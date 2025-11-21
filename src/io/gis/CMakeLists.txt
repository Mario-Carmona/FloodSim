add_library(flood_gis
    GdalReader.cpp
)

target_include_directories(flood_gis PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(flood_gis
    PRIVATE GDAL::GDAL
            fmt::fmt
)