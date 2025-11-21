add_executable(FloodSimApp
    main.cpp
)

target_link_libraries(FloodSimApp
    PRIVATE flood_core
            flood_gis
            flood_mqtt
            flood_taste
            flood_gui
            cxxopts::cxxopts
            TBB::tbb
            fmt::fmt
)