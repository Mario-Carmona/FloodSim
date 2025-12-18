
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

# ---------------------------------------------------------
# External Dependencies via vcpkg
# ---------------------------------------------------------

# GDAL
find_package(GDAL CONFIG REQUIRED)

# Tensorflow C 
include(${CMAKE_SOURCE_DIR}/cmake/tensorflow.cmake)


# ------------------------------------------------------------
# Model Generation
# ------------------------------------------------------------
set(MODEL_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/export_model.py")

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


add_subdirectory(Common)
add_subdirectory(IO)
add_subdirectory(Core)
add_subdirectory(UI)

add_subdirectory(Main)
