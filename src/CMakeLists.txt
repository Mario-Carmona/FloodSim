
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

# ---------------------------------------------------------
# Python Virtual Environment Automation
# ---------------------------------------------------------
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(VENV_PATH "${CMAKE_SOURCE_DIR}/.venv")

if(WIN32)
    set(PYTHON_VENV_EXE "${VENV_PATH}/Scripts/python.exe")
    set(PIP_VENV_EXE "${VENV_PATH}/Scripts/pip.exe")
else()
    set(PYTHON_VENV_EXE "${VENV_PATH}/bin/python")
    set(PIP_VENV_EXE "${VENV_PATH}/bin/pip")
endif()

# Create venv if it doesn't exist 
if(NOT EXISTS "${VENV_PATH}")
    message(STATUS "Creating Python virtual environment...")
    execute_process(COMMAND "${Python3_EXECUTABLE}" -m venv "${VENV_PATH}")
endif()

# Install dependencies if requirements.txt changed or stamp is missing
message(STATUS "Installing Python dependencies...")
execute_process(
    COMMAND "${PIP_VENV_EXE}" install -r "${CMAKE_SOURCE_DIR}/requirements.txt"
    RESULT_VARIABLE pip_result
    OUTPUT_VARIABLE pip_stdout
    ERROR_VARIABLE pip_stderr
)

# Show the error only if something went wrong
if(NOT pip_result EQUAL 0)
    message(FATAL_ERROR "Error instalando dependencias: ${pip_stderr}")
endif()



# ------------------------------------------------------------
# Model Generation
# ------------------------------------------------------------
set(MODEL_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/tensorflow/export_model.py")

if(NOT EXISTS "${MODEL_SCRIPT}")
    message(FATAL_ERROR 
        "Required model export script not found at: ${MODEL_SCRIPT}\n"
        "Please ensure the script exists before configuring the project."
    )
else()
    message(STATUS "Generating/Overwriting TensorFlow Model")
    execute_process(
        COMMAND "${PYTHON_VENV_EXE}" "${MODEL_SCRIPT}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE export_result
    )
    
    if(NOT export_result EQUAL 0)
        message(FATAL_ERROR "Failed to generate the TensorFlow .pb model.")
    endif()
endif()



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
set(GENERATED_DIR_CPP "${CMAKE_SOURCE_DIR}/include/proto")
set(GENERATED_DIR_PYTHON "${CMAKE_SOURCE_DIR}/scripts/mqtt/libs")

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
