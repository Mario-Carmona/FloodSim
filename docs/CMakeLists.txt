
# Buscar Doxygen en el entorno
find_package(Doxygen)

if (DOXYGEN_FOUND)
    # 1. Copiamos LICENSE a la carpeta de build dándole extensión .md para Doxygen
    configure_file("${CMAKE_SOURCE_DIR}/LICENSE" "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.md" COPYONLY)
    configure_file("${CMAKE_SOURCE_DIR}/THIRD-PARTY-NOTICES.md" "${CMAKE_CURRENT_BINARY_DIR}/THIRD-PARTY-NOTICES.md" COPYONLY)

    # Estas variables sobreescriben la configuración por defecto
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/docs)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "MANUAL.md")

    # Crear un target llamado 'docs' para invocar Doxygen
    doxygen_add_docs(docs
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        MANUAL.md
        "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.md"
        "${CMAKE_CURRENT_BINARY_DIR}/THIRD-PARTY-NOTICES.md"
        COMMENT "Generando documentación web con Doxygen"
    )

    # 🚀 GENERACIÓN AUTOMÁTICA DE .NOJEKYLL
    # Por defecto, Doxygen crea una carpeta llamada 'html' dentro del OUTPUT_DIRECTORY.
    # Usamos 'POST_BUILD' para que el archivo se cree justo DESPUÉS de que Doxygen termine.
    add_custom_command(TARGET docs POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_SOURCE_DIR}/docs/html/.nojekyll"
        COMMENT "Creando archivo .nojekyll en la raíz de la web para GitHub Pages"
    )

else()
    message(FATAL_ERROR "Doxygen no encontrado")
endif()
