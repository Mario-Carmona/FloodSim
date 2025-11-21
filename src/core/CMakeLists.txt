# Capa CORE — simulador puro
add_library(flood_core
    Grid.cpp
    Simulator.cpp
    HydrologyModel.cpp
    FloodPropagation.cpp
)

target_include_directories(flood_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(flood_core
    PRIVATE fmt::fmt
            nlohmann_json::nlohmann_json
            magic_enum::magic_enum
)