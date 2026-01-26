
add_library(danasim_output_mqtt_payload)

target_sources(danasim_output_mqtt_payload
    PRIVATE
        ProtobufSerializer.cpp
)

target_include_directories(danasim_output_mqtt_payload
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_output_mqtt_payload
    PUBLIC
        danasim_options
        proto_messages_lib
)
