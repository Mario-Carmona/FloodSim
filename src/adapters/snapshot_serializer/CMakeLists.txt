
add_library(danasim_snapshot_serializer_adapters)

target_sources(danasim_snapshot_serializer_adapters
    PRIVATE
        ProtobufSnapshotSerializer.cpp
        X3DSnapshotSerializer.cpp
        X3DOptimizedSnapshotSerializer.cpp
)

target_include_directories(danasim_snapshot_serializer_adapters
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(danasim_snapshot_serializer_adapters
    PUBLIC
        danasim_options
        danasim_core
        ZLIB::ZLIB
        proto_messages_lib
)
