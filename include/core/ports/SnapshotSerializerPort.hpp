
#pragma once

#include <string>
#include <filesystem>

#include "core/snapshot/Snapshot.hpp"

namespace danasim {

    class SnapshotSerializerPort {
    public:
        virtual ~SnapshotSerializerPort() = default;

        virtual void serializeToStr(const Snapshot& snapshot, std::string& result) const = 0;
        virtual void serializeToFile(const Snapshot& snapshot, const std::filesystem::path& path) const = 0;

    protected:
        virtual void serializeToStream(std::ostream& os, const Snapshot& snapshot) const = 0;
    };

} // namespace danasim
