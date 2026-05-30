
# =========================================================
# 📤 Data Sink (Output) Adapters Configuration
# =========================================================

# ---------------------------------------------------------
# 📦 External Telemetry & Computer Vision Dependencies
# ---------------------------------------------------------
# Explicitly route the OpenCV root to the vcpkg installation directory
set(OpenCV_ROOT "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/opencv4")

# Resolve Computer Vision framework for visual state serialization
find_package(OpenCV REQUIRED)

# Resolve IoT/Message Broker dependencies for real-time telemetry publishing
find_package(PahoMqttCpp CONFIG REQUIRED)

# ---------------------------------------------------------
# 📂 Internal Module Hierarchy Traversals
# ---------------------------------------------------------
# Register the internal MQTT client and broker communication submodule
add_subdirectory(mqtt)

# ---------------------------------------------------------
# 🏗️ Core Library Component Declaration
# ---------------------------------------------------------
# Define the concrete adapter library responsible for bridging the core 
# simulation domain with external data sinks and serialization boundaries.
add_library(floodsim_output_adapters)

# ---------------------------------------------------------
# 🎯 Source Compilation Units Registration
# ---------------------------------------------------------
target_sources(floodsim_output_adapters
    PRIVATE
        # Creational instantiation
        OutputFactory.cpp
        
        # Concrete driven adapters
        CheckpointOutput.cpp # Simulation state snapshot handlers
        MQTTOutput.cpp       # Real-time telemetry publishers
        ImageOutput.cpp      # Visual frame serialization handlers
)

# ---------------------------------------------------------
# 📂 Header Search Directories
# ---------------------------------------------------------
# Expose the global project include root and Computer Vision headers
target_include_directories(floodsim_output_adapters
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
)

# ---------------------------------------------------------
# 🔗 Linkage & Dependency Propagation
# ---------------------------------------------------------
# Bind core logic, internal serialization writers, and the external OpenCV runtime
target_link_libraries(floodsim_output_adapters
    PUBLIC
        floodsim_options
        floodsim_core
        floodsim_io_writers
        floodsim_output_mqtt
        ${OpenCV_LIBS}
)

# ---------------------------------------------------------
# Platform-Specific Linkage Directives (MQTT)
# ---------------------------------------------------------
# Adapt MQTT linkage strategies based on the target OS deployment model
if(WIN32)
    # Windows targets dynamically link the MQTT client (shared libraries)
    target_link_libraries(floodsim_output_adapters
        PUBLIC
            PahoMqttCpp::paho-mqttpp3 PahoMqttCpp::paho-mqttpp3-shared
    )
else()
    # Unix-like environments favor static linkage for the MQTT client
    target_link_libraries(floodsim_output_adapters
        PUBLIC
            PahoMqttCpp::paho-mqttpp3 PahoMqttCpp::paho-mqttpp3-static
    )
endif()
