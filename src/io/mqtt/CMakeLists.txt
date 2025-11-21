add_library(flood_mqtt
    MqttClient.cpp
)

target_include_directories(flood_mqtt PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Solo enlazar si está disponible
if(TARGET paho-mqttpp3)
    target_link_libraries(flood_mqtt PRIVATE paho-mqttpp3 paho-mqtt3as)
else()
    message(WARNING "MQTT libraries not found, MQTT adapter will not be available.")
endif()