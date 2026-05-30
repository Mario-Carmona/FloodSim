/**
 * @file JsonSerializer.cpp
 * @brief Implementation of the JsonSerializer class.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "app/adapters/output/mqtt/payload/JsonSerializer.hpp"

#include <chrono>
#include <string>

#include <fmt/chrono.h>
#include <fmt/core.h>

#include "logging/Logger.hpp"

namespace floodsim::app::adapters::output {

std::string JsonSerializer::GeneratePingPayload(const std::string& client_id,
    const std::string& timestamp_utc) const {
    nlohmann::json payload = {
        {"process", "System_Ping"},
        {"source", client_id},
        {"timestamp_utc", timestamp_utc}
    };

    return payload.dump();
}

nlohmann::json JsonSerializer::ParsePongPayload(const std::string& payload) const {
    return nlohmann::json::parse(payload);
}

std::string JsonSerializer::GenerateInitMapConfigPayload(const core::grid::MapGrid& grid,
                                                         std::chrono::sys_seconds start_timestamp) const {

    config::ViewBox::Point georef = grid.GetGeoreference();
        
    nlohmann::json payload = {
        {"process", "InitMap_Config"},
        {"map", {
            {"size_x", grid.GetCols()},
            {"size_y", grid.GetRows()},
            {"chunk_size", 100},
            {"cell_resolution_m", grid.GetCellSize()}
        }},
        {"metadata", {
            {"sim_start_time", fmt::format("{:%Y-%m-%dT%H:%M:%S}", start_timestamp)},
            {"time_step_s", grid.GetScalar<float>("delta_t")->GetValue()}
        }},
        {"georeference", {
            {"lat", georef.lat},
            {"lon", georef.lon}
        }}
    };

    return payload.dump();
}

std::string JsonSerializer::GenerateInitAgentLayerPayload(const std::string& dataset_name,
                                                          const std::string& layer_name) const {
    nlohmann::json payload = {
        {"process", "InitAgent_Layer"},
        {"id", layer_name},
        {"data_path", dataset_name + "/" + layer_name},
        {"data_filename", layer_name}
    };

    return payload.dump();
}

std::string JsonSerializer::GenerateInitAgentEOFPayload() const {
    nlohmann::json payload = {
        {"process", "InitAgent_EOF"}
    };

    return payload.dump();
}

std::string JsonSerializer::GenerateInitEOFPayload(GridIndexType total_chunks) const {
    nlohmann::json payload = {
        {"process", "Init_EOF"},
        {"total_chunks_sent", total_chunks}
    };

    return payload.dump();
}

std::string JsonSerializer::GenerateFrameStartPayload(GridIndexType total_chunks,
                                                      GridIndexType chunks_per_batch) const {
    nlohmann::json payload = {
        {"process", "FrameStart"},
        {"total_chunks", total_chunks},
        {"chunks_per_batch", chunks_per_batch}
    };

    return payload.dump();
}

void JsonSerializer::GenerateChunkChangesPayload(std::string& payload_str,
                                                 const std::vector<float>& water_depth,
                                                 const std::vector<int8_t>& flood_risk,
                                                 const std::string& layer_name,
                                                 const core::snapshot::ChangeList& changes,
                                                 GridIndexType init_index,
                                                 GridIndexType chunk_size) const {
    nlohmann::json payload = {
        {"process", "EYE_SetState_Layer"},
        {"id", layer_name},
        {"changes", {
            {"coord", {{"x", 0}, {"y", 0}, {"z", 0}}},
            {"cells", nlohmann::json::object()}
        }}
    };

    for (GridIndexType i = init_index; i < (init_index + chunk_size); ++i) {
        GridIndexType index = changes.indexes[i];
            
        payload["changes"]["cells"][std::to_string(index)] = {
                {"state", flood_risk[index]},
                {"height", water_depth[index]}
        };
    }

    payload_str = payload.dump();
}

nlohmann::json JsonSerializer::ParseChunkAckPayload(const std::string& payload) const {
    return nlohmann::json::parse(payload);
}

std::string JsonSerializer::GenerateFrameEndPayload() const {
    nlohmann::json payload = {
        {"process", "FrameEnd"}
    };

    return payload.dump();
}

std::string JsonSerializer::GenerateFrameSyncPayload(std::chrono::sys_seconds time) const {
    nlohmann::json payload = {
        {"process", "EYE_Frame_Sync"},
        {"simulation_time", fmt::format("{:%Y-%m-%dT%H:%M:%S}", time)}
    };

    return payload.dump();
}

std::string JsonSerializer::GenerateSimEndPayload() const {
    nlohmann::json payload = {
        {"process", "Sim_End"},
        {"sim_time_total", 3600.0}
    };

    return payload.dump();
}

} // namespace floodsim::app::adapters::output
