
# =========================================================
# 🤖 Machine Learning Models Pipeline Configuration
# =========================================================

# ---------------------------------------------------------
# 🔍 Build-Time Source Discovery & Script Resolution
# ---------------------------------------------------------
# Recursively gather all Python model version definitions during configuration.
file(GLOB_RECURSE MODEL_SOURCES "${CMAKE_SOURCE_DIR}/python/models/versions/*.py")

# Define the absolute path to the core model exporting orchestrator script
set(EXPORT_SCRIPT "${CMAKE_SOURCE_DIR}/python/models/export_models.py")

# ---------------------------------------------------------
# ⏱️ State Tracking & Witness Token Definition
# ---------------------------------------------------------
# Define a witness/timestamp file to track generation states and avoid redundant executions
set(TIMESTAMP_FILE "${CMAKE_SOURCE_DIR}/exported_models/.export_timestamp")

# ---------------------------------------------------------
# ⚙️ Automated Model Generation Command
# ---------------------------------------------------------
# Register an automated custom command that triggers whenever python source models or 
# the exporter script undergo modifications.
add_custom_command(
    OUTPUT ${TIMESTAMP_FILE}
    COMMAND ${MODELS_ENV_PYTHON} ${EXPORT_SCRIPT}
    DEPENDS ${EXPORT_SCRIPT} ${MODEL_SOURCES}
    COMMENT "Inspecting Python model definitions and re-exporting neural assets..."
    VERBATIM
)

# ---------------------------------------------------------
# 🎯 Custom Generation Target Pipeline
# ---------------------------------------------------------
# Construct a top-level target integrated into the default build graph ('ALL') 
# to enforce the asset export execution loop.
add_custom_target(export_models_target ALL DEPENDS ${TIMESTAMP_FILE})

# ---------------------------------------------------------
# 📦 Platform-Specific Layout Configurations
# ---------------------------------------------------------
# Adapt asset deployment locations according to target operating system filesystem paradigms
if(WIN32)
    set(EXPORTED_MODELS_DEST_PATH "${CMAKE_INSTALL_DATADIR}")
else()
    set(EXPORTED_MODELS_DEST_PATH "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME_LOWER}")
endif()

# ---------------------------------------------------------
# 🚀 Granular Component Deployment
# ---------------------------------------------------------
# Deploy the entirely generated asset directory to the designated layout
install(DIRECTORY "${CMAKE_SOURCE_DIR}/exported_models"
        DESTINATION ${EXPORTED_MODELS_DEST_PATH}
        COMPONENT Unspecified
        PATTERN ".export_timestamp" EXCLUDE
)
