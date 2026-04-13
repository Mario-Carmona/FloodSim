
# ONNX Runtime
include(${CMAKE_SOURCE_DIR}/cmake/onnxruntime.cmake)



add_library(danasim_state_updater_adapters)

target_sources(danasim_state_updater_adapters
    PRIVATE
        OnnxStateUpdater.cpp
        StateUpdaterFactory.cpp
)

target_include_directories(danasim_state_updater_adapters
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_state_updater_adapters
    PUBLIC
        danasim_options
        danasim_core
        onnxruntime
)
