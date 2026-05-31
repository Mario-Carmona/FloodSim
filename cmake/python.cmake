
# ---------------------------------------------------------
# 🐍 Python Virtual Environment Automation
# ---------------------------------------------------------
find_package(Python3 3.10 EXACT REQUIRED COMPONENTS Interpreter)

# --- Function to bootstrap isolated Python virtual environments ---
function(create_python_env ENV_NAME ISOLATE_ENVIRONMENT)
    set(PYTHONPATH "${CMAKE_SOURCE_DIR}/python")

    # Define the target path for the virtual environment (typically within build/venvs)
    set(VENV_PATH "$ENV{HOST_BUILD_DIR}/venvs/${ENV_NAME}_env")
    
    # Detect the OS-specific virtual environment executables and paths
    if(WIN32)
        set(VENV_BIN_DIR "${VENV_PATH}/Scripts")
        set(VENV_PYTHON "${VENV_BIN_DIR}/python.exe")
        set(VENV_PIP "${VENV_BIN_DIR}/pip.exe")
    else()
        set(VENV_BIN_DIR "${VENV_PATH}/bin")
        set(VENV_PYTHON "${VENV_BIN_DIR}/python")
        set(VENV_PIP "${VENV_BIN_DIR}/pip")
    endif()

    # 1. Bootstrap the environment only if it does not already exist
    if(NOT EXISTS "${VENV_PYTHON}")
        message(STATUS "Bootstrapping Python virtual environment: ${ENV_NAME}...")

        set(VENV_FLAGS "")
        if(NOT ISOLATE_ENVIRONMENT)
            list(APPEND VENV_FLAGS "--system-site-packages")
        endif()

        if(WIN32)
            # Windows: Utilize the native 'venv' module (standard and stable)
            execute_process(
                COMMAND "${Python3_EXECUTABLE}" -m venv ${VENV_PATH} ${VENV_FLAGS}
                RESULT_VARIABLE _res
            )
        else()
            # Linux: Utilize 'virtualenv' to mitigate environment constraints (e.g., Ubuntu/Kaggle)
            # Attempt to locate a pre-installed virtualenv executable
            find_program(VIRTUALENV_EXE "virtualenv")

            if(NOT VIRTUALENV_EXE)
                execute_process(
                    COMMAND "${Python3_EXECUTABLE}" -m pip install --quiet virtualenv
                    RESULT_VARIABLE _res_virtualenv
                )

                execute_process(
                    COMMAND "${Python3_EXECUTABLE}" -m pip install wrapt --upgrade --ignore-installed
                    RESULT_VARIABLE _res_wrapt
                )

                find_program(VIRTUALENV_EXE "virtualenv")
            endif()

            # Invoke the virtualenv executable to provision the environment
            execute_process(
                COMMAND "${VIRTUALENV_EXE}" ${VENV_PATH} ${VENV_FLAGS}
                RESULT_VARIABLE _res
            )
        endif()

        if(NOT _res EQUAL 0)
            message(FATAL_ERROR "Failed to bootstrap virtual environment ${ENV_NAME}. Exit code: ${_res}")
        endif()

        message(STATUS "Configuring custom PYTHONPATH injection for ${ENV_NAME}...")
        
        # Generate a dynamic .pth file to inject project-specific paths into the environment
        if(WIN32)
            # Windows environments map site-packages directly under the 'Lib' directory
            set(SITE_PACKAGES_DIR "${VENV_PATH}/Lib/site-packages")
        else()
            # Linux environments map site-packages under 'lib/pythonX.Y'
            # CMake resolves the version dynamically via find_package(Python3)
            set(SITE_PACKAGES_DIR "${VENV_PATH}/lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages")
        endif()

        # Ensure the target site-packages directory exists before writing the .pth file
        file(MAKE_DIRECTORY "${SITE_PACKAGES_DIR}")

        message(STATUS "Injecting custom PYTHONPATH into: ${SITE_PACKAGES_DIR}")
        
        # Write the .pth configuration to the dynamically resolved site-packages path
        file(WRITE "${SITE_PACKAGES_DIR}/${ENV_NAME}_env_custom.pth" "${PYTHONPATH}")
    endif()

    set(REQS_FILE "${PYTHONPATH}/${ENV_NAME}/requirements.txt")

    # 2. Install/Update dependencies (Always execute to ensure state consistency)
    if(EXISTS "${REQS_FILE}")
        message(STATUS "Installing dependencies for ${ENV_NAME} via ${REQS_FILE}...")

        # Preemptively upgrade pip to prevent deprecation warnings and guarantee compatibility
        execute_process(
            COMMAND "${VENV_PYTHON}" -m pip install --upgrade pip --quiet
        )

        execute_process(
            COMMAND "${VENV_PIP}" install -r "${REQS_FILE}"
            OUTPUT_QUIET # Suppress standard output (remove to debug pip installation logs)
            RESULT_VARIABLE _res
        )
        if(NOT _res EQUAL 0)
            message(WARNING "An error occurred while installing dependencies for ${ENV_NAME}")
        endif()
    endif()

    # 3. Export environment variables for downstream target consumption
    # Promotes the environment's python executable path to the parent scope using an uppercase convention (e.g., TF_ENV_PYTHON)
    string(TOUPPER "${ENV_NAME}_env" ENV_NAME_UPPER)
    set(${ENV_NAME_UPPER}_PYTHON "${VENV_PYTHON}" PARENT_SCOPE)

endfunction()
