
add_library(danasim_core)

target_sources(danasim_core
    PRIVATE
        simulation/SimulationCore.cpp
        snapshot/SnapshotManager.cpp
        snapshot/Snapshot.cpp
        grid/MapGrid.cpp
        grid/StreamedLayerManager.cpp
)

target_include_directories(danasim_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_core 
    PUBLIC 
        danasim_options
        magic_enum::magic_enum
        HighFive
)
