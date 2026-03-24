
# ONNX Runtime
include(${CMAKE_SOURCE_DIR}/cmake/onnxruntime.cmake)



# --- Creación de los dos entornos ---

# 1. Entorno "Builder" (TensorFlow)
create_python_env("tensorflow_scripts" ON)



# 1. Localizar todos los archivos .py actuales (se actualiza cada vez que ejecutas CMake)
file(GLOB_RECURSE MODEL_SOURCES "${CMAKE_SOURCE_DIR}/python/tensorflow_scripts/models/*.py")
set(EXPORT_SCRIPT "${CMAKE_SOURCE_DIR}/python/tensorflow_scripts/export_models.py")

# 2. Definir el archivo "testigo" en la carpeta de salida
set(TIMESTAMP_FILE "${CMAKE_SOURCE_DIR}/exported_models/.export_timestamp")

# 3. Comando automatizado
add_custom_command(
    OUTPUT ${TIMESTAMP_FILE}
    COMMAND ${TENSORFLOW_SCRIPTS_ENV_PYTHON} ${EXPORT_SCRIPT}
    DEPENDS ${EXPORT_SCRIPT} ${MODEL_SOURCES}
    COMMENT "Buscando nuevos modelos .py o cambios en existentes..."
    VERBATIM
)

# 4. Creamos un target intermedio para el comando
add_custom_target(export_models_target ALL DEPENDS ${TIMESTAMP_FILE})



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

add_dependencies(danasim_state_updater_adapters export_models_target)
