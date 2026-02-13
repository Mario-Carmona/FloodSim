
#pragma once

#include <string>
#include <filesystem>

#include "core/snapshot/Snapshot.hpp"
#include "core/snapshot/ChangeList.hpp"

namespace danasim {

    class PayloadSerializer {
    public:
        virtual ~PayloadSerializer() = default;

        virtual void serializeChunk(const Snapshot& snapshot, const ChangeList& changes, const int32_t chunkIndex, const int32_t totalChunks, const size_t chunkSize, std::string& result) const = 0;
    };

} // namespace danasim
