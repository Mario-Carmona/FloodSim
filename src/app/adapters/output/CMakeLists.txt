

set(OpenCV_ROOT "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/opencv4")
find_package(OpenCV REQUIRED)


find_package(PahoMqttCpp CONFIG REQUIRED)



add_subdirectory(mqtt)

add_library(danasim_output_adapters)

target_sources(danasim_output_adapters
    PRIVATE
        CheckpointOutput.cpp
        MQTTOutput.cpp
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
        danasim_io_writers
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
