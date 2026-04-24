
#pragma once

#include "adapters/output/mqtt/payload/PayloadSerializer.hpp"

namespace danasim {

    class JsonSerializer final : public PayloadSerializer {
    public:
        virtual std::string generatePingPayload(const std::string& clientID, const std::string& timestampUTC) const override;
        virtual nlohmann::json parsePongPayload(const std::string& payload) const override;

        virtual std::string generateInitMapConfigPayload(const MapGrid& grid, std::chrono::sys_seconds startTimestamp) const override;
        virtual std::string generateInitAgentLayerPayload(const MapGrid& grid, const std::string& datasetName, const std::string& layerName) const override;
        virtual std::string generateInitAgentEOFPayload() const override;
        virtual std::string generateInitEOFPayload(GridIndexType totalChunks) const override;

        virtual std::string generateFrameStartPayload(GridIndexType totalChunks, GridIndexType chunks_per_batch) const;
        virtual void generateChunkChangesPayload(std::string& payloadStr, const std::vector<float>& water_depth, const std::vector<int8_t>& flood_risk, const std::string& layerName, const ChangeList& changes, GridIndexType initIndex, GridIndexType chunkSize) const override;
        virtual nlohmann::json parseChunkAckPayload(const std::string& payload) const;
        virtual std::string generateFrameEndPayload() const;

        virtual std::string generateFrameSyncPayload(std::chrono::sys_seconds time) const;

        virtual std::string generateSimEndPayload() const;
    };

} // namespace danasim
