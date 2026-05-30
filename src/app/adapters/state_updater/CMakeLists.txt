
# =========================================================
# 🔄 State Transition (State Updater) Adapters Configuration
# =========================================================

# ---------------------------------------------------------
# 🤖 External AI/ML Engine Integration
# ---------------------------------------------------------
# Resolve and inject the ONNX Runtime C++ API for neural network inference
include(${CMAKE_SOURCE_DIR}/cmake/onnxruntime.cmake)

# Define the concrete adapter library responsible for advancing the 
# core simulation state using machine-learning-driven models.
add_library(floodsim_state_updater_adapters)

# ---------------------------------------------------------
# 🎯 Source Compilation Units Registration
# ---------------------------------------------------------
# Register the factory for creational instantiation and the concrete 
# ONNX-based state transition implementation.
target_sources(floodsim_state_updater_adapters
    PRIVATE
        OnnxStateUpdater.cpp
        StateUpdaterFactory.cpp
)

# ---------------------------------------------------------
# 📂 Header Search Directories
# ---------------------------------------------------------
# Expose the global project include root to all downstream dependents
target_include_directories(floodsim_state_updater_adapters
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# ---------------------------------------------------------
# 🔗 Linkage & Dependency Propagation
# ---------------------------------------------------------
# Bind the core simulation logic, global policies, and the ONNX runtime 
# engine into this adapter boundary.
target_link_libraries(floodsim_state_updater_adapters
    PUBLIC
        floodsim_options
        floodsim_core
        onnxruntime
)
