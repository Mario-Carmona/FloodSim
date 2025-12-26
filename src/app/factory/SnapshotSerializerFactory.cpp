
#include "app/factory/SnapshotSerializerFactory.hpp"

#include "adapters/snapshot_serializer/ProtobufSnapshotSerializer.hpp"

namespace danasim {

    std::unique_ptr<SnapshotSerializerPort>
        SnapshotSerializerFactory::create(const OutputConfig::MQTTFormat& format)
    {
        switch (format) {
        case OutputConfig::MQTTFormat::PROTOBUF: {
            return std::make_unique<ProtobufSnapshotSerializer>();
            break;
        }
        default: {
            throw std::runtime_error("Unknown snapshot format");
            break;
        }
        }
    }

} // namespace danasim
