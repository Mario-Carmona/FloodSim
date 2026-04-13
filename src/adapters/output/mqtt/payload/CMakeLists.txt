

find_package(nlohmann_json CONFIG REQUIRED)



add_library(danasim_output_mqtt_payload)

target_sources(danasim_output_mqtt_payload
    PRIVATE
        JsonSerializer.cpp
)

target_include_directories(danasim_output_mqtt_payload
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_output_mqtt_payload
    PUBLIC
        danasim_options
        nlohmann_json::nlohmann_json
)
