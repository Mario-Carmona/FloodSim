
add_executable(DanaSimCLI
    main_cli.cpp
)

target_link_libraries(DanaSimCLI PRIVATE
    danasim_options
    DanaSim::UI::CLI
)
