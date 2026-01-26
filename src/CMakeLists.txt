
# ---------------------------------------------------------
# Common External Dependencies via vcpkg
# ---------------------------------------------------------

# GDAL
find_package(GDAL CONFIG REQUIRED)

# YAML
find_package(yaml-cpp REQUIRED)

# Tensorflow C 
include(${CMAKE_SOURCE_DIR}/cmake/tensorflow.cmake)

# spdlog
find_package(spdlog CONFIG REQUIRED)

# magic-enum
find_package(magic_enum CONFIG REQUIRED)

# ZLIB
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

# paho-mqttpp3
find_package(PahoMqttCpp CONFIG REQUIRED)

# Protobuf
find_package(Protobuf CONFIG REQUIRED)

# HighFive
find_package(HighFive CONFIG REQUIRED)

# OpenCV
set(OpenCV_ROOT "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/opencv4")
find_package(OpenCV REQUIRED)

# ---------------------------------------------------------
# Python Virtual Environment Automation
# ---------------------------------------------------------
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# --- Función para crear entornos Python aislados ---
function(create_python_env ENV_NAME)
    set(PYTHONPATH "${CMAKE_SOURCE_DIR}/python")

    # Definir la ruta donde vivirá el entorno (dentro de build/venvs)
    set(VENV_PATH "${CMAKE_SOURCE_DIR}/build/venvs/${ENV_NAME}_env")
    
    # Detectar ejecutable del entorno virtual según S.O.
    if(WIN32)
        set(VENV_PYTHON "${VENV_PATH}/Scripts/python.exe")
        set(VENV_PIP "${VENV_PATH}/Scripts/pip.exe")
    else()
        set(VENV_PYTHON "${VENV_PATH}/bin/python")
        set(VENV_PIP "${VENV_PATH}/bin/pip")
    endif()

    # 1. Crear el entorno solo si no existe
    if(NOT EXISTS "${VENV_PYTHON}")
        message(STATUS "Creando entorno virtual Python: ${ENV_NAME}...")
        execute_process(
            COMMAND "${Python3_EXECUTABLE}" -m venv "${VENV_PATH}"
            RESULT_VARIABLE _res
        )
        if(NOT _res EQUAL 0)
            message(FATAL_ERROR "Fallo al crear venv ${ENV_NAME}")
        endif()

        message(STATUS "Configurando PYTHONPATH extra para ${ENV_NAME}...")
        
        # Creamos un archivo temporal en la carpeta de build con el contenido
        file(WRITE "${VENV_PATH}/Lib/site-packages/${ENV_NAME}_env_custom.pth" "${PYTHONPATH}")
    endif()

    set(REQS_FILE "${PYTHONPATH}/${ENV_NAME}/requirements.txt")

    # 2. Instalar requerimientos (siempre se intenta para actualizar cambios)
    if(EXISTS "${REQS_FILE}")
        message(STATUS "Instalando dependencias para ${ENV_NAME} desde ${REQS_FILE}...")
        execute_process(
            COMMAND "${VENV_PIP}" install -r "${REQS_FILE}"
            OUTPUT_QUIET # Quita esto si quieres ver el log de pip
            RESULT_VARIABLE _res
        )
        if(NOT _res EQUAL 0)
            message(WARNING "Hubo un error instalando dependencias para ${ENV_NAME}")
        endif()
    endif()

    # 3. Exportar la variable para usarla luego en el proyecto
    # Crea una variable con el nombre del entorno en mayúsculas (ej: TF_ENV_PYTHON)
    string(TOUPPER "${ENV_NAME}_env" ENV_NAME_UPPER)
    set(${ENV_NAME_UPPER}_PYTHON "${VENV_PYTHON}" PARENT_SCOPE)

endfunction()



# --- Creación de los dos entornos ---

# 1. Entorno "Builder" (TensorFlow)
create_python_env("tensorflow_scripts")

# 2. Entorno "Monitor" (MQTT + Protobuf Moderno)
create_python_env("mqtt_scripts")

# 3. Entorno "Data"
create_python_env("sim_data_scripts")





# ------------------------------------------------------------
# Model Generation
# ------------------------------------------------------------

# 1. Definir variables de rutas para claridad
set(TF_SCRIPT "${CMAKE_SOURCE_DIR}/python/tensorflow_scripts/export_model.py")
set(COMMON_SCRIPT "${CMAKE_SOURCE_DIR}/python/tensorflow_scripts/constants_loader.py")
set(CONSTANTS_HPP "${CMAKE_SOURCE_DIR}/include/core/common/SimulationConstants.hpp")
set(MODEL_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/models")
set(MODEL_SENTINEL "${MODEL_OUTPUT_DIR}/saved_model.pb") # Archivo testigo que crea Tensorflow


# 2. CREAR EL COMANDO (La lógica inteligente)
# Esto dice: "Para obtener 'saved_model.pb', ejecuta este comando Python".
# Solo se ejecutará si falta el .pb O si alguno de los DEPENDS ha cambiado.
add_custom_command(
    OUTPUT "${MODEL_SENTINEL}"
    
    # El comando a ejecutar (usa el python detectado)
    COMMAND ${TENSORFLOW_SCRIPTS_ENV_PYTHON} "${TF_SCRIPT}"
    
    # DEPENDENCIAS: Si alguno de estos cambia, se regenera el modelo
    DEPENDS "${TF_SCRIPT}" "${COMMON_SCRIPT}" "${CONSTANTS_HPP}" "${REQ_TF}"
    
    # Opcional: El directorio de trabajo
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    
    COMMENT "Comprobando cambios en el modelo TensorFlow..."
    VERBATIM
)

# 3. CREAR EL TARGET (El envoltorio)
# Creamos un "objetivo" abstracto que depende del archivo de salida anterior.
add_custom_target(generate_model_tf DEPENDS "${MODEL_SENTINEL}")



# ------------------------------------------------------------
# Protobuf files Generation
# ------------------------------------------------------------

# ==============================================================================
# 2. DEFINIR RUTAS Y ARCHIVOS
# ==============================================================================
# Directorio donde está el .proto
# Rutas
set(PROTO_DIR "${CMAKE_SOURCE_DIR}/proto")
set(PROTO_FILE "${PROTO_DIR}/messages.proto")
set(GENERATED_DIR_CPP "${CMAKE_SOURCE_DIR}/include/adapters/output/mqtt/payload/proto")
set(GENERATED_DIR_PYTHON "${CMAKE_SOURCE_DIR}/python/mqtt_scripts/libs")

# Archivos de salida esperados
set(PROTO_SRCS "${GENERATED_DIR_CPP}/messages.pb.cc") # Nota: protoc suele replicar la estructura de carpetas 'schema/'
set(PROTO_HDRS "${GENERATED_DIR_CPP}/messages.pb.h")
set(PROTO_PY   "${GENERATED_DIR_PYTHON}/messages_pb2.py")

file(MAKE_DIRECTORY ${GENERATED_DIR_CPP})
file(MAKE_DIRECTORY ${GENERATED_DIR_PYTHON})

add_custom_command(
    OUTPUT ${PROTO_SRCS} ${PROTO_HDRS} ${PROTO_PY}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS --proto_path=${PROTO_DIR}
         --cpp_out=${GENERATED_DIR_CPP}
         --python_out=${GENERATED_DIR_PYTHON}
         ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Compilando esquema Protobuf ..."
    VERBATIM
)

# ==============================================================================
# 4. CREAR UNA LIBRERÍA INTERMEDIA PARA C++
# ==============================================================================

add_library(proto_messages_lib STATIC ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(proto_messages_lib PUBLIC protobuf::libprotobuf)



# =========================================================
# DEFINICIÓN DE OPCIONES GLOBALES (INTERFACE TARGET)
# =========================================================
add_library(danasim_options INTERFACE)

# 1. Opciones de COMPILACIÓN (Compile Flags)
target_compile_features(danasim_options INTERFACE cxx_std_20)

if(WIN32)
    target_compile_definitions(danasim_options INTERFACE 
        NOMINMAX
        WIN32_LEAN_AND_MEAN # Opcional: Acelera la compilación en Windows
    )

    target_compile_options(danasim_options INTERFACE
        # DEBUG
        $<$<CONFIG:Debug>:/Od /Zi /RTC1 /W4 /utf-8>
        # RELEASE
        $<$<CONFIG:Release>:/O2 /DNDEBUG /GL /W4 /utf-8>
    )
    
    # 2. Opciones de LINKER (Solo MSVC suele necesitar esto para LTCG)
    target_link_options(danasim_options INTERFACE
        $<$<CONFIG:Debug>:/DEBUG>
        $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
    )
else()
    # GCC / Clang
    target_compile_options(danasim_options INTERFACE
        $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
    )
endif()





# -----------------------
# Subproyectos
# -----------------------
add_subdirectory(core)
add_subdirectory(adapters)
add_subdirectory(app)
add_subdirectory(cli)

if(DANASIM_BUILD_GUI)
    add_subdirectory(gui)
endif()



# =========================================================
# CONFIGURACIÓN DE EMPAQUETADO (CPack)
# =========================================================

# SOLUCIÓN: Carga el módulo de CPack para inicializar todas las variables
include(CPack) 

# (Opcional, pero recomendado para instaladores de Windows)
# Incluye las librerías de tiempo de ejecución del sistema necesarias (como los runtimes de MSVC)
# para que el instalador WIX/MSI las pueda incluir.
include(InstallRequiredSystemLibraries)

# =========================================================
