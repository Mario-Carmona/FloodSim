
# =========================================================
# 🌐 MQTT Telemetry Submodule Configuration
# =========================================================

# ---------------------------------------------------------
# 📂 Internal Module Hierarchy Traversals
# ---------------------------------------------------------
# Register the payload construction and data formatting submodule
add_subdirectory(payload)

# ---------------------------------------------------------
# 🏗️ Interface Target Declaration
# ---------------------------------------------------------
# Define a structural interface library for MQTT telemetry operations.
# This target acts as an umbrella component to aggregate internal MQTT 
# handlers and formatters without compiling source files locally.
add_library(floodsim_output_mqtt INTERFACE)

# ---------------------------------------------------------
# 🔗 Linkage & Dependency Propagation
# ---------------------------------------------------------
# Consolidate internal payload dependencies and global build policies, 
# propagating them upstream to the core output adapters layer.
target_link_libraries(floodsim_output_mqtt
    INTERFACE
        floodsim_options
        floodsim_output_mqtt_payload
)
