
# 1. Localizar todos los archivos .py actuales (se actualiza cada vez que ejecutas CMake)
file(GLOB_RECURSE MODEL_SOURCES "${CMAKE_SOURCE_DIR}/python/models/versions/*.py")
set(EXPORT_SCRIPT "${CMAKE_SOURCE_DIR}/python/models/export_models.py")

# 2. Definir el archivo "testigo" en la carpeta de salida
set(TIMESTAMP_FILE "${CMAKE_SOURCE_DIR}/exported_models/.export_timestamp")

# 3. Comando automatizado
add_custom_command(
    OUTPUT ${TIMESTAMP_FILE}
    COMMAND ${MODELS_ENV_PYTHON} ${EXPORT_SCRIPT}
    DEPENDS ${EXPORT_SCRIPT} ${MODEL_SOURCES}
    COMMENT "Buscando nuevos modelos .py o cambios en existentes..."
    VERBATIM
)

# 4. Creamos un target intermedio para el comando
add_custom_target(export_models_target ALL DEPENDS ${TIMESTAMP_FILE})



if(WIN32)
    # En Windows: C:\Archivos de Programa\FloodSim\config
    set(EXPORTED_MODELS_DEST_PATH "${CMAKE_INSTALL_DATADIR}")
else()
    # En Linux: /usr/share/floodsim/config (Estándar para archivos estáticos)
    # Opcional: Si son archivos que el usuario debe editar, usa "etc/floodsim"
    set(EXPORTED_MODELS_DEST_PATH "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME_LOWER}")
endif()

# Instalar el directorio completo
install(DIRECTORY "${CMAKE_SOURCE_DIR}/exported_models"
        DESTINATION ${EXPORTED_MODELS_DEST_PATH}
        COMPONENT Unspecified # O pon "GUI" si solo la usa la interfaz
)
