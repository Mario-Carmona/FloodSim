

set(OpenCV_ROOT "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/opencv4")
find_package(OpenCV REQUIRED)


find_package(PahoMqttCpp CONFIG REQUIRED)




# 2. Entorno "Monitor" (MQTT + Protobuf Moderno)
create_python_env("mqtt_scripts" OFF)




add_subdirectory(mqtt)

add_library(danasim_output_adapters)

target_sources(danasim_output_adapters
    PRIVATE
        X3dFileOutput.cpp
        MqttOutput.cpp
        ImageOutput.cpp
        OutputFactory.cpp
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
        ${OpenCV_LIBS}
)

if(WIN32)
    target_link_libraries(danasim_output_adapters
        PUBLIC
            PahoMqttCpp::paho-mqttpp3 PahoMqttCpp::paho-mqttpp3-shared
    )
else()
    target_link_libraries(danasim_output_adapters
        PUBLIC
            PahoMqttCpp::paho-mqttpp3 PahoMqttCpp::paho-mqttpp3-static
    )
endif()
