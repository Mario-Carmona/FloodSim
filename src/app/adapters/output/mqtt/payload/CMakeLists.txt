
# =========================================================
# 📜 MQTT Telemetry Payload Serialization Submodule
# =========================================================

# ---------------------------------------------------------
# 📦 Dependency Resolution
# ---------------------------------------------------------
# Resolve the external industry-standard JSON abstraction and parsing library
find_package(nlohmann_json CONFIG REQUIRED)

# ---------------------------------------------------------
# 🏗️ Core Library Component Declaration
# ---------------------------------------------------------
# Define the concrete library responsible for formatting and serializing 
# simulation metrics into structured network payloads.
add_library(floodsim_output_mqtt_payload)

# ---------------------------------------------------------
# 🎯 Source Compilation Units Registration
# ---------------------------------------------------------
target_sources(floodsim_output_mqtt_payload
    PRIVATE
        JsonSerializer.cpp
)

# ---------------------------------------------------------
# 📂 Header Search Directories
# ---------------------------------------------------------
# Expose the global project include root to all downstream dependents
target_include_directories(floodsim_output_mqtt_payload
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# ---------------------------------------------------------
# 🔗 Linkage & Dependency Propagation
# ---------------------------------------------------------
# Inherit global build policies and inject the JSON serialization engine 
# to propagate payload generation capabilities upstream to the MQTT layer.
target_link_libraries(floodsim_output_mqtt_payload
    PUBLIC
        floodsim_options
        nlohmann_json::nlohmann_json
)
