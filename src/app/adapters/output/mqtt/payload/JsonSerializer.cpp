
#include "app/adapters/output/mqtt/payload/JsonSerializer.hpp"

#include <fmt/core.h>
#include <fmt/chrono.h>

#include <chrono>
#include <string>

#include "logging/Logger.hpp"

namespace danasim {

    std::string JsonSerializer::generatePingPayload(const std::string& clientID, const std::string& timestampUTC) const {
        nlohmann::json payload = {
            {"process", "System_Ping"},
            {"source", clientID},
            {"timestamp_utc", timestampUTC}
        };

        return payload.dump();
    }

    nlohmann::json JsonSerializer::parsePongPayload(const std::string& payload) const {
        return nlohmann::json::parse(payload);
    }

    std::string JsonSerializer::generateInitMapConfigPayload(const MapGrid& grid, std::chrono::sys_seconds startTimestamp) const {
        ViewBox::Point georef = grid.georeference();
        
        nlohmann::json payload = {
            {"process", "InitMap_Config"},
            {"map", {
                {"size_x", grid.cols()},
                {"size_y", grid.rows()},
                {"chunk_size", 100},
                {"cell_resolution_m", grid.cellSize()}
            }},
            {"metadata", {
                {"sim_start_time", fmt::format("{:%Y-%m-%dT%H:%M:%S}", startTimestamp)},
                {"time_step_s", grid.getScalar<float>("delta_t")->getValue()}
            }},
            {"georeference", {
                {"lat", georef.lat},
                {"lon", georef.lon}
            }}
        };

        return payload.dump();
    }

    std::string JsonSerializer::generateInitAgentLayerPayload(const MapGrid& grid, const std::string& datasetName, const std::string& layerName) const {
        nlohmann::json payload = {
            {"process", "InitAgent_Layer"},
            {"id", layerName},
            {"data_path", datasetName + "/" + layerName},
            {"data_filename", layerName}
        };

        return payload.dump();
    }

    std::string JsonSerializer::generateInitAgentEOFPayload() const {
        nlohmann::json payload = {
            {"process", "InitAgent_EOF"}
        };

        return payload.dump();
    }

    std::string JsonSerializer::generateInitEOFPayload(GridIndexType totalChunks) const {
        nlohmann::json payload = {
            {"process", "Init_EOF"},
            {"total_chunks_sent", totalChunks}
        };

        return payload.dump();
    }

    std::string JsonSerializer::generateFrameStartPayload(GridIndexType totalChunks, GridIndexType chunks_per_batch) const {
        nlohmann::json payload = {
            {"process", "FrameStart"},
            {"total_chunks", totalChunks},
            {"chunks_per_batch", chunks_per_batch}
        };

        return payload.dump();
    }

    void JsonSerializer::generateChunkChangesPayload(std::string& payloadStr, const std::vector<float>& water_depth, const std::vector<int8_t>& flood_risk, const std::string& layerName, const ChangeList& changes, GridIndexType initIndex, GridIndexType chunkSize) const {
        nlohmann::json payload = {
            {"process", "EYE_SetState_Layer"},
            {"id", layerName},
            {"changes", {
                {"coord", {{"x", 0}, {"y", 0}, {"z", 0}}},
                {"cells", nlohmann::json::object()}
            }}
        };

        for (GridIndexType i = initIndex; i < (initIndex + chunkSize); ++i) {
            GridIndexType index = changes.indexes[i];
            
            payload["changes"]["cells"][std::to_string(index)] = {
                    {"state", flood_risk[index]},
                    {"height", water_depth[index]}
            };
        }

        payloadStr = payload.dump();
    }

    nlohmann::json JsonSerializer::parseChunkAckPayload(const std::string& payload) const {
        return nlohmann::json::parse(payload);
    }

    std::string JsonSerializer::generateFrameEndPayload() const {
        nlohmann::json payload = {
            {"process", "FrameEnd"}
        };

        return payload.dump();
    }

    std::string JsonSerializer::generateFrameSyncPayload(std::chrono::sys_seconds time) const {
        nlohmann::json payload = {
            {"process", "EYE_Frame_Sync"},
            {"simulation_time", fmt::format("{:%Y-%m-%dT%H:%M:%S}", time)}
        };

        return payload.dump();
    }

    std::string JsonSerializer::generateSimEndPayload() const {
        nlohmann::json payload = {
            {"process", "Sim_End"},
            {"sim_time_total", 3600.0}
        };

        return payload.dump();
    }

} // namespace danasim
