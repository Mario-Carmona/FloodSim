
find_package(Protobuf CONFIG REQUIRED)


# ------------------------------------------------------------
# Protobuf files Generation
# ------------------------------------------------------------

# ==============================================================================
# 2. DEFINIR RUTAS Y ARCHIVOS
# ==============================================================================
# Directorio donde está el .proto
# Rutas
set(PROTO_DIR "${CMAKE_SOURCE_DIR}/proto")
set(PROTO_FILE "${PROTO_DIR}/messages.proto")
set(GENERATED_DIR_CPP "${CMAKE_SOURCE_DIR}/include/adapters/output/mqtt/payload/proto")
set(GENERATED_DIR_PYTHON "${CMAKE_SOURCE_DIR}/python/mqtt_scripts/libs")

# Archivos de salida esperados
set(PROTO_SRCS "${GENERATED_DIR_CPP}/messages.pb.cc") # Nota: protoc suele replicar la estructura de carpetas 'schema/'
set(PROTO_HDRS "${GENERATED_DIR_CPP}/messages.pb.h")
set(PROTO_PY   "${GENERATED_DIR_PYTHON}/messages_pb2.py")

file(MAKE_DIRECTORY ${GENERATED_DIR_CPP})
file(MAKE_DIRECTORY ${GENERATED_DIR_PYTHON})

add_custom_command(
    OUTPUT ${PROTO_SRCS} ${PROTO_HDRS} ${PROTO_PY}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS --proto_path=${PROTO_DIR}
         --cpp_out=${GENERATED_DIR_CPP}
         --python_out=${GENERATED_DIR_PYTHON}
         ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Compilando esquema Protobuf ..."
    VERBATIM
)

# ==============================================================================
# 4. CREAR UNA LIBRERÍA INTERMEDIA PARA C++
# ==============================================================================

add_library(proto_messages_lib STATIC ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(proto_messages_lib PUBLIC protobuf::libprotobuf)




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
