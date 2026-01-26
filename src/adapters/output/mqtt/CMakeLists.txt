
add_subdirectory(payload)

add_library(danasim_output_mqtt INTERFACE)

target_link_libraries(danasim_output_mqtt
    INTERFACE
        danasim_options
        danasim_output_mqtt_payload
)
