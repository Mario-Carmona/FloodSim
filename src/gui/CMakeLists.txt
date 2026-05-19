
find_package(OpenGL REQUIRED)

find_package(imgui CONFIG REQUIRED)

find_package(glfw3 CONFIG REQUIRED)


find_path(PORTABLE_FILE_DIALOGS_INCLUDE_DIRS "portable-file-dialogs.h")



add_subdirectory(tabs)


if(WIN32)
    # Compilamos con mainGUI.cpp y el archivo de recursos .rc
    add_executable(${GUI_EXE_NAME} mainGUI.cpp app_icon.rc)
else()
    # En Linux no se usan archivos .rc
    add_executable(${GUI_EXE_NAME} mainGUI.cpp)
endif()


set_target_properties(${GUI_EXE_NAME} PROPERTIES DEBUG_POSTFIX "d")

if(WIN32)
    # 1. Mantenemos que sea un ejecutable de Windows (sin consola de fondo)
    set_target_properties(${GUI_EXE_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)

    # 2. Forzamos al enlazador a buscar 'main' aunque el subsistema sea WINDOWS
    # Esto resuelve el error LNK2001 al mapear WinMain -> main
    target_link_options(${GUI_EXE_NAME} PRIVATE "/ENTRY:mainCRTStartup")
endif()

target_link_libraries(${GUI_EXE_NAME}
    PRIVATE
        danasim_options
        danasim_app
        danasim_gui_tabs
        imgui::imgui
        glfw
        OpenGL::GL
)

target_include_directories(${GUI_EXE_NAME} 
    PRIVATE 
        ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
)

# Instalación asignada al componente GUI
install(TARGETS ${GUI_EXE_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT GUI # <--- Distingue este binario del CLI
)
