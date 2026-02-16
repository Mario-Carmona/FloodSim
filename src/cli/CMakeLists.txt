
add_executable(danasim_cli
    main_cli.cpp
)

target_link_libraries(danasim_cli
    PRIVATE
        danasim_options
        danasim_app
)

set_target_properties(danasim_cli PROPERTIES
    OUTPUT_NAME DanaSimCLI
    DEBUG_POSTFIX "d"
)
