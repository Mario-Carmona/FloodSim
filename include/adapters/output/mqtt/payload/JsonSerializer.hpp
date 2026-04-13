
#pragma once

#include "adapters/output/mqtt/payload/PayloadSerializer.hpp"

namespace danasim {

    class JsonSerializer final : public PayloadSerializer {
    public:
        virtual void serializeChunk(const Snapshot& snapshot, const ChangeList& changes, const int32_t chunkIndex, const int32_t totalChunks, const size_t chunkSize, std::string& result) const override;
    };

} // namespace danasim
