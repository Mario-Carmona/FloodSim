
cmake_minimum_required(VERSION 4.1.1)


if(BUILD_APP)
    set(ENABLE_INPUT_ADAPTERS ON)
    set(ENABLE_OUTPUT_ADAPTERS ON)
    set(ENABLE_STATE_UPDATER_ADAPTERS ON)
    set(ENABLE_APP ON)
    set(ENABLE_CORE ON)
endif()

if(BUILD_ALL_TESTS)
    set(ENABLE_STATE_UPDATER_ADAPTERS ON)
    set(ENABLE_CORE ON)
endif()


set(VCPKG_MANIFEST_FEATURES "")

if(ENABLE_INPUT_ADAPTERS)
    list(APPEND VCPKG_MANIFEST_FEATURES "input-adapters")
endif()

if(ENABLE_OUTPUT_ADAPTERS)
    list(APPEND VCPKG_MANIFEST_FEATURES "output-adapters")
endif()

if(ENABLE_APP)
    list(APPEND VCPKG_MANIFEST_FEATURES "app")
endif()

if(ENABLE_CORE)
    list(APPEND VCPKG_MANIFEST_FEATURES "app-core")
endif()

if(BUILD_APP)
    if(ENABLE_GUI)
        list(APPEND VCPKG_MANIFEST_FEATURES "gui")
    endif()
endif()

if(BUILD_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES "test")
endif()



# 1. Detectar la variable de entorno y crear la carpeta si existe
if(DEFINED ENV{VCPKG_DEFAULT_BINARY_CACHE})
    file(MAKE_DIRECTORY "$ENV{VCPKG_DEFAULT_BINARY_CACHE}")
endif()



# ------------------------------------------------------------
# 🕒 Generación de versión compatible con Windows (MSI)
# ------------------------------------------------------------
string(TIMESTAMP CURRENT_YEAR "%Y")
string(TIMESTAMP CURRENT_MONTH "%m")
string(TIMESTAMP CURRENT_DAY "%d")

# Convertimos el año a un número menor a 256 (ej: 2026 -> 26)
math(EXPR MAJOR_VER "${CURRENT_YEAR} - 2000")
# Eliminamos ceros a la izquierda para evitar que CMake los interprete como octales
math(EXPR MINOR_VER "${CURRENT_MONTH} + 0")
math(EXPR PATCH_VER "${CURRENT_DAY} + 0")

set(PROGRAM_VERSION "${MAJOR_VER}.${MINOR_VER}.${PATCH_VER}")

# Opcional: Mantener la fecha completa solo para mostrar en logs o UI
set(FULL_DATE_VERSION "${CURRENT_YEAR}.${CURRENT_MONTH}.${CURRENT_DAY}")




project(FloodSim VERSION ${PROGRAM_VERSION} LANGUAGES CXX)




if(BUILD_APP OR BUILD_TESTS)
    add_subdirectory(src)
endif()

if(BUILD_TESTS)
    enable_testing()

    add_subdirectory(tests)
endif()

if(BUILD_PYTHON_ENVS)
    add_subdirectory(python)
endif()



if(BUILD_APP)

    # Definir dónde se guardará la configuración según el sistema operativo
    if(WIN32)
        # En Windows: C:\Archivos de Programa\FloodSim\config
        set(CONFIG_DEST_PATH "config")
    else()
        # En Linux: /usr/share/floodsim/config (Estándar para archivos estáticos)
        # Opcional: Si son archivos que el usuario debe editar, usa "etc/floodsim"
        set(CONFIG_DEST_PATH "etc/floodsim/config")
    endif()

    # Instalar el directorio completo
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/config/"
            DESTINATION ${CONFIG_DEST_PATH}
            COMPONENT Unspecified # O pon "GUI" si solo la usa la interfaz
    )


    # Información básica del paquete
    if(ENABLE_GUI)
        set(CPACK_PACKAGE_NAME "${PROJECT_NAME}-Desktop-Full")
    else()
        set(CPACK_PACKAGE_NAME "${PROJECT_NAME}-Server-CLI")
    endif()
    
    set(CPACK_PACKAGE_VENDOR "Tu Nombre o Compañía")
    set(CPACK_PACKAGE_VERSION ${PROGRAM_VERSION})
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Simulador de Redes Neuronales y Procesamiento de Datos")


    # Obtener la arquitectura del procesador (amd64 / x86_64)
    set(ARCH ${CMAKE_SYSTEM_PROCESSOR})

    if(WIN32)
        # Nombre para Windows
        set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${PROJECT_VERSION}_Windows_${ARCH}")
    
    elseif(UNIX AND NOT APPLE)
        # Si estamos en Linux, intentamos sacar la versión exacta del SO (ej. "Ubuntu-22.04")
        # leyendo el archivo /etc/os-release
        set(OS_RELEASE_NAME "Linux")
    
        if(EXISTS "/etc/os-release")
            file(READ "/etc/os-release" OS_RELEASE_CONTENT)
            string(REGEX MATCH "PRETTY_NAME=\"([^\"]+)\"" _ ${OS_RELEASE_CONTENT})
            if(CMAKE_MATCH_1)
                # Reemplaza espacios por guiones (Ej: "Ubuntu 22.04.3 LTS" -> "Ubuntu-22.04.3-LTS")
                string(REPLACE " " "-" OS_RELEASE_NAME "${CMAKE_MATCH_1}") 
            endif()
        endif()

        # Nombre para Linux
        set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${PROJECT_VERSION}_${OS_RELEASE_NAME}_${ARCH}")
    endif()


    # Separar por componentes
    set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)

    # 1. Añadimos Unspecified a la lista de componentes 
    list(APPEND CPACK_COMPONENTS_ALL "Unspecified")
    
    # Opcional: Descripción de componentes para NSIS/WIX
    set(CPACK_COMPONENT_CLI_DISPLAY_NAME "Herramientas de Línea de Comandos")
    set(CPACK_COMPONENT_GUI_DISPLAY_NAME "Interfaz Gráfica de Usuario")

    # 2. Hacemos que el componente de dependencias sea obligatorio e invisible
    set(CPACK_COMPONENT_UNSPECIFIED_HIDDEN TRUE)
    set(CPACK_COMPONENT_UNSPECIFIED_REQUIRED TRUE)
    
    # Permitir que las DLLs de vcpkg se instalen como parte del componente
    set(CPACK_APP_COMPONENT_INSTALL ON)
    
    if(WIN32)
        # 1. Definimos que las librerías de sistema (msvcp140, etc.) 
        # se instalen en los componentes que nosotros queremos.
        set(CPACK_SYSTEM_LIBRARIES_ID "Unspecified")

        # Forzamos a CPack a ejecutar la instalación para cada componente
        # Estructura: "Directorio de Build" "Nombre Proyecto" "Componente" "Directorio Instalación"
        # 3. Forzamos a CPack a ejecutar la instalación también para Unspecified [cite: 6]
        set(CPACK_INSTALL_CMAKE_PROJECTS 
            "${CMAKE_BINARY_DIR}" "${PROJECT_NAME}" "CLI" "/"
            "${CMAKE_BINARY_DIR}" "${PROJECT_NAME}" "GUI" "/"
            "${CMAKE_BINARY_DIR}" "${PROJECT_NAME}" "Unspecified" "/"
        )
    
        # 2. Incluimos el módulo (Esto genera la lista de DLLs de VS)
        include(InstallRequiredSystemLibraries)

        # 3. Configuración visual del instalador NSIS/WIX
        set(CPACK_PACKAGE_EXECUTABLES 
            "FloodSimGUI" "FloodSim Simulator"
            "FloodSimCLI" "FloodSim Terminal"
        )
    endif()


    # Configuración específica para Windows
    if(WIN32)

        set(CPACK_GENERATOR "WIX")

        # Ruta típica de instalación de WiX v3
        set(WIX_ROOT "C:/Program Files (x86)/WiX Toolset v3.14/bin")
    
        # Indicamos a CMake dónde buscar los ejecutables específicos
        find_program(CPACK_WIX_CANDLE_EXECUTABLE candle PATHS ${WIX_ROOT} NO_DEFAULT_PATH)
        find_program(CPACK_WIX_LIGHT_EXECUTABLE  light  PATHS ${WIX_ROOT} NO_DEFAULT_PATH)
        

        # 1. Genera un GUID nuevo una sola vez y pégalo aquí permanentemente
        set(CPACK_WIX_UPGRADE_GUID "86D6D602-CCC2-4BE3-A83C-4D9D7A61FAE0")


        set(CPACK_WIX_PRODUCT_ICON "${CMAKE_CURRENT_SOURCE_DIR}/res/floodsim.ico")

        # ---------------------------------------------------------
        # 1. CARPETA EN EL MENÚ DE INICIO (ESTILO PROFESIONAL)
        # ---------------------------------------------------------
        # Crea una carpeta "Tu Compañía -> FloodSim" en Inicio
        set(CPACK_WIX_PROGRAM_MENU_FOLDER "FloodSim")

        # ---------------------------------------------------------
        # 2. ACCESOS DIRECTOS EN EL MENÚ DE INICIO
        # ---------------------------------------------------------
        # Formato: "NombreDelTarget" "Nombre Amigable a Mostrar"
        # IMPORTANTE: El primer valor debe ser el nombre exacto de tu add_executable()
        set(CPACK_PACKAGE_EXECUTABLES 
            "${PROJECT_NAME}GUI" "FloodSim Simulator"
            "${PROJECT_NAME}CLI" "FloodSim Terminal"
        )

    endif()

    # Configuración específica para Linux
    if(UNIX AND NOT APPLE)
        set(CPACK_GENERATOR "DEB") # Genera un instalador .deb y un comprimido .tar.gz

        # 1. Instalar el acceso directo (Menú de aplicaciones)
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/res/linux/floodsim.desktop"
            DESTINATION "share/applications"
            COMPONENT GUI # Solo lo instalamos si se instala la interfaz gráfica
        )

        # 2. Instalar el icono de la aplicación
        # (Asegúrate de tener un floodsim.png de 256x256 píxeles)
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/res/floodsim.svg"
            DESTINATION "share/icons/hicolor/256x256/apps"
            COMPONENT GUI
        )

        # Datos del paquete
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Tu Nombre <tu-email@ejemplo.com>")
        set(CPACK_DEBIAN_PACKAGE_SECTION "science")

        # Prefijo para los paquetes (ej. floodsim-gui_1.0.0_amd64.deb)
        set(CPACK_DEBIAN_PACKAGE_NAME "floodsim")

        # ---------------------------------------------------------
        # GENERACIÓN UNIFICADA (TODA LA SUITE EN UN SOLO .DEB)
        # ---------------------------------------------------------
        # Al poner esto en OFF, CPack ignorará las separaciones de COMPONENT
        # y meterá la GUI, la CLI y las librerías en un único paquete.
        set(CPACK_DEB_COMPONENT_INSTALL OFF)
    endif()

    # Incluir CPack (siempre al final)
    include(CPack)
endif()
