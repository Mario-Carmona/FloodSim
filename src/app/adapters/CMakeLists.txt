
# =========================================================
# 🔌 Architectural Adapters Module Configuration
# =========================================================

# ---------------------------------------------------------
# 📂 Conditional Module Hierarchy Traversals
# ---------------------------------------------------------
# Conditionally register adapter submodules based on global build flags

# Ingestion adapters for external data sources
if(ENABLE_INPUT_ADAPTERS)
    add_subdirectory(input)
endif()

# Serialization adapters for external data sinks
if(ENABLE_OUTPUT_ADAPTERS)
    add_subdirectory(output)
endif()

# State transition adapters for core simulation logic updates
if(ENABLE_STATE_UPDATER_ADAPTERS)
    add_subdirectory(state_updater)
endif()

# ---------------------------------------------------------
# 🏗️ Consolidated Interface Target Declaration
# ---------------------------------------------------------
# If all architectural adapter components are explicitly enabled, 
# construct a unified interface target. This acts as an umbrella 
# module to propagate all adapter dependencies upstream to the application core.
if(ENABLE_INPUT_ADAPTERS AND ENABLE_OUTPUT_ADAPTERS AND ENABLE_STATE_UPDATER_ADAPTERS)
    add_library(floodsim_adapters INTERFACE)

    # ---------------------------------------------------------
    # 🔗 Linkage & Dependency Propagation
    # ---------------------------------------------------------
    # Consolidate individual adapter layers into a single architectural boundary
    target_link_libraries(floodsim_adapters
        INTERFACE
            floodsim_input_adapters
            floodsim_output_adapters
            floodsim_state_updater_adapters
    )
endif()
