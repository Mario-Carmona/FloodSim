
add_executable(danasim_gui
    main_gui.cpp
)

target_link_libraries(danasim_gui
    PRIVATE
        danasim_options
        danasim_app
        danasim_input_adapters
        danasim_output_adapters
        danasim_snapshot_serializer_adapters
        danasim_solver_adapters
)

set_target_properties(danasim_gui PROPERTIES
    OUTPUT_NAME DanaSimGUI
    DEBUG_POSTFIX "d"
)
