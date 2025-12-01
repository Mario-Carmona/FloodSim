
add_library(danasim_common
    SimulationBridge.cpp
)

target_include_directories(danasim_common PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

add_library(DanaSim::Common ALIAS danasim_common)
