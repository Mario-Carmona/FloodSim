
#pragma once

#include <string>
#include <filesystem>

#include <nlohmann/json.hpp>

#include "core/snapshot/Snapshot.hpp"
#include "core/snapshot/ChangeList.hpp"

#include "core/grid/MapGrid.hpp"

namespace danasim {

    class PayloadSerializer {
    public:
        virtual ~PayloadSerializer() = default;

        virtual std::string generatePingPayload(const std::string& clientID, const std::string& timestampUTC) const = 0;
        virtual nlohmann::json parsePongPayload(const std::string& payload) const = 0;

        virtual std::string generateInitMapConfigPayload(const MapGrid& grid, std::chrono::sys_seconds startTimestamp) const = 0;
        virtual std::string generateInitAgentLayerPayload(const MapGrid& grid, const std::string& datasetName, const std::string& layerName) const = 0;
        virtual std::string generateInitAgentEOFPayload() const = 0;
        virtual std::string generateInitEOFPayload(GridIndexType totalChunks) const = 0;

        virtual std::string generateFrameStartPayload(GridIndexType totalChunks, GridIndexType chunks_per_batch) const = 0;
        virtual void generateChunkChangesPayload(std::string& payloadStr, const std::vector<float>& water_depth, const std::vector<int8_t>& flood_risk, const std::string& layerName, const ChangeList& changes, GridIndexType initIndex, GridIndexType chunkSize) const = 0;
        virtual nlohmann::json parseChunkAckPayload(const std::string& payload) const = 0;
        virtual std::string generateFrameEndPayload() const = 0;

        virtual std::string generateFrameSyncPayload(std::chrono::sys_seconds time) const = 0;

        virtual std::string generateSimEndPayload() const = 0;
    };

} // namespace danasim
