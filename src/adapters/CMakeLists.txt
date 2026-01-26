
add_subdirectory(input)
add_subdirectory(output)
add_subdirectory(state_updater)


add_library(danasim_adapters INTERFACE)

target_link_libraries(danasim_adapters
    INTERFACE
        danasim_input_adapters
        danasim_output_adapters
        danasim_state_updater_adapters
)
