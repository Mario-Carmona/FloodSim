
add_library(danasim_output_adapters)

target_sources(danasim_output_adapters
    PRIVATE
        X3DFileOutput.cpp
        MQTTOutput.cpp
)

target_include_directories(danasim_output_adapters
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_output_adapters
    PUBLIC
        danasim_options
        danasim_core
        danasim_snapshot_serializer_adapters
        PahoMqttCpp::paho-mqttpp3 PahoMqttCpp::paho-mqttpp3-shared
)
