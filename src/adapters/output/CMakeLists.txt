
add_subdirectory(mqtt)

add_library(danasim_output_adapters)

target_sources(danasim_output_adapters
    PRIVATE
        X3DFileOutput.cpp
        MQTTOutput.cpp
        ImageOutput.cpp
)

target_include_directories(danasim_output_adapters
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(danasim_output_adapters
    PUBLIC
        danasim_options
        danasim_core
        danasim_output_mqtt
        PahoMqttCpp::paho-mqttpp3 PahoMqttCpp::paho-mqttpp3-shared
        ${OpenCV_LIBS}
)
