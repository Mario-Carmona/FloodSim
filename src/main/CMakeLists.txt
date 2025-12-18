
add_executable(DanaSimCLI
    mainCLI.cpp
)

set_target_properties(DanaSimCLI PROPERTIES
    DEBUG_POSTFIX "d"
)

target_link_libraries(DanaSimCLI PRIVATE
    danasim_options
    DanaSim::UI::CLI
)
