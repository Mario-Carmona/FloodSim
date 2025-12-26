
#pragma once

#include "core/ports/SnapshotSerializerPort.hpp"

namespace danasim {

    /**
     * @brief JSON serializer for snapshots.
     */
    class ProtobufSnapshotSerializer final : public SnapshotSerializerPort {
    public:
        void serializeToStr(const Snapshot& snapshot, std::string& result) const override;
        void serializeToFile(const Snapshot& snapshot, const std::filesystem::path& path) const override;

    protected:
        void serializeToStream(std::ostream& os, const Snapshot& snapshot) const override;
    };

} // namespace danasim
