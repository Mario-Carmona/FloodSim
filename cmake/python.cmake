
# ---------------------------------------------------------
# Python Virtual Environment Automation
# ---------------------------------------------------------
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# --- Función para crear entornos Python aislados ---
function(create_python_env ENV_NAME ISOLATE_ENVIRONMENT)
    set(PYTHONPATH "${CMAKE_SOURCE_DIR}/python")

    # Definir la ruta donde vivirá el entorno (dentro de build/venvs)
    set(VENV_PATH "${CMAKE_SOURCE_DIR}/build/venvs/${ENV_NAME}_env")
    
    # Detectar ejecutable del entorno virtual según S.O.
    if(WIN32)
        set(VENV_BIN_DIR "${VENV_PATH}/Scripts")
        set(VENV_PYTHON "${VENV_BIN_DIR}/python.exe")
        set(VENV_PIP "${VENV_BIN_DIR}/pip.exe")
    else()
        set(VENV_BIN_DIR "${VENV_PATH}/bin")
        set(VENV_PYTHON "${VENV_BIN_DIR}/python")
        set(VENV_PIP "${VENV_BIN_DIR}/pip")
    endif()

    # 1. Crear el entorno solo si no existe
    if(NOT EXISTS "${VENV_PYTHON}")
        message(STATUS "Creando entorno virtual Python: ${ENV_NAME}...")

        set(VENV_FLAGS "")
        if(NOT ISOLATE_ENVIRONMENT)
            list(APPEND VENV_FLAGS "--system-site-packages")
        endif()

        if(WIN32)
            # EN WINDOWS: Usamos el módulo nativo 'venv' (es lo estándar y seguro)
            execute_process(
                COMMAND "${Python3_EXECUTABLE}" -m venv ${VENV_PATH} ${VENV_FLAGS}
                RESULT_VARIABLE _res
            )
        else()
            # EN LINUX: Usamos 'virtualenv' (soluciona el problema de Kaggle/Ubuntu)
            # Primero intentamos encontrar el ejecutable virtualenv
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

            # Si existe el comando 'virtualenv', lo usamos directo
            execute_process(
                COMMAND "${VIRTUALENV_EXE}" ${VENV_PATH} ${VENV_FLAGS}
                RESULT_VARIABLE _res
            )
        endif()

        if(NOT _res EQUAL 0)
            message(FATAL_ERROR "Fallo al crear venv ${ENV_NAME}. Código: ${_res}")
        endif()

        message(STATUS "Configurando PYTHONPATH extra para ${ENV_NAME}...")
        
        # Creamos un archivo temporal en la carpeta de build con el contenido
        if(WIN32)
            # En Windows la carpeta es "Lib" (con mayúscula) y directa
            set(SITE_PACKAGES_DIR "${VENV_PATH}/Lib/site-packages")
        else()
            # En Linux es "lib" (minúscula) e incluye la versión (ej: python3.10)
            # CMake ya sabe la versión gracias a find_package(Python3)
            set(SITE_PACKAGES_DIR "${VENV_PATH}/lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages")
        endif()

        # Nos aseguramos de que la carpeta exista antes de escribir
        file(MAKE_DIRECTORY "${SITE_PACKAGES_DIR}")

        message(STATUS "Configurando PYTHONPATH extra en: ${SITE_PACKAGES_DIR}")
        
        # Escribimos el archivo .pth en la ruta calculada dinámicamente
        file(WRITE "${SITE_PACKAGES_DIR}/${ENV_NAME}_env_custom.pth" "${PYTHONPATH}")
    endif()

    set(REQS_FILE "${PYTHONPATH}/${ENV_NAME}/requirements.txt")

    # 2. Instalar requerimientos (siempre se intenta para actualizar cambios)
    if(EXISTS "${REQS_FILE}")
        message(STATUS "Instalando dependencias para ${ENV_NAME} desde ${REQS_FILE}...")

        # Siempre actualizamos pip primero para evitar warnings
        execute_process(
            COMMAND "${VENV_PYTHON}" -m pip install --upgrade pip --quiet
        )

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
