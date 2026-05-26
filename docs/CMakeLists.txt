
# Buscar Doxygen en el entorno
find_package(Doxygen)

if (DOXYGEN_FOUND)
    # Estas variables sobreescriben la configuración por defecto
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/docs)
    set(DOXYGEN_GENERATE_HTML YES)
    #set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "MANUAL.md")

    # Crear un target llamado 'docs' para invocar Doxygen
    doxygen_add_docs(docs
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        COMMENT "Generando documentación web con Doxygen"
    )
else()
    message(FATAL_ERROR "Doxygen no encontrado")
endif()
