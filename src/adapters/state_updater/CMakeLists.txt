
add_library(danasim_state_updater_adapters)

target_sources(danasim_state_updater_adapters
    PRIVATE
        TensorFlowStateUpdater.cpp
)

target_include_directories(danasim_state_updater_adapters
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_state_updater_adapters
    PUBLIC
        danasim_options
        danasim_core
        tensorflow_c
)

# 4. ENLAZAR CON TU LIBRERÍA (La conexión)
# Esto fuerza a CMake a asegurar que 'generate_model_tf' esté listo 
# ANTES de empezar a compilar 'danasim_core'.
add_dependencies(danasim_state_updater_adapters generate_model_tf)
