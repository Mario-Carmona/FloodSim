
set(CLI_EXE_NAME "${PROJECT_NAME}CLI")


if(WIN32)
    # Compilamos con mainCLI.cpp y el archivo de recursos .rc
    add_executable(${CLI_EXE_NAME} mainCLI.cpp app_icon.rc)
else()
    # En Linux no se usan archivos .rc
    add_executable(${CLI_EXE_NAME} mainCLI.cpp)
endif()


set_target_properties(${CLI_EXE_NAME} PROPERTIES DEBUG_POSTFIX "d")

target_link_libraries(${CLI_EXE_NAME}
    PRIVATE
        danasim_options
        danasim_app
)

# Instalación profesional por componentes
install(TARGETS ${CLI_EXE_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT CLI  # <--- Crucial para el empaquetado filtrado
)
