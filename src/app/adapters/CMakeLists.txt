
if(ENABLE_INPUT_ADAPTERS)
    add_subdirectory(input)
endif()

if(ENABLE_OUTPUT_ADAPTERS)
    add_subdirectory(output)
endif()

if(ENABLE_STATE_UPDATER_ADAPTERS)
    add_subdirectory(state_updater)
endif()

if(ENABLE_INPUT_ADAPTERS AND ENABLE_OUTPUT_ADAPTERS AND ENABLE_STATE_UPDATER_ADAPTERS)
    add_library(danasim_adapters INTERFACE)

    target_link_libraries(danasim_adapters
        INTERFACE
            danasim_input_adapters
            danasim_output_adapters
            danasim_state_updater_adapters
    )
endif()
