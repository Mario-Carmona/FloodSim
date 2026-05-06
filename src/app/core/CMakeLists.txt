

# magic-enum
find_package(magic_enum CONFIG REQUIRED)

find_package(GeographicLib CONFIG REQUIRED)




add_library(danasim_core)

target_sources(danasim_core
    PRIVATE
        SimulationCore.cpp
        snapshot/SnapshotManager.cpp
        snapshot/Snapshot.cpp
        grid/MapGrid.cpp
)

target_include_directories(danasim_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_core 
    PUBLIC 
        danasim_options
        magic_enum::magic_enum
        ${GeographicLib_LIBRARIES}
)
