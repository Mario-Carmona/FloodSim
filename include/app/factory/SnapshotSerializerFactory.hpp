
#pragma once

#include <memory>
#include "app/config/Config.hpp"
#include "core/ports/SnapshotSerializerPort.hpp"

namespace danasim {

    class SnapshotSerializerFactory {
    public:
        static std::unique_ptr<SnapshotSerializerPort>
            create(const OutputConfig::MQTTFormat& format);
    };

} // namespace danasim
