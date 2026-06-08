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

namespace {

/**
 * @brief Converts a hexadecimal color string to RGBA values.
 * @param hex_str The hexadecimal color string.
 * @return A vector containing the RGBA values.
 */
std::vector<int> HexToRgba(std::string hex_str) {
    // Remove the ‘#’ if it is present
    if (!hex_str.empty() && hex_str[0] == '#') {
        hex_str = hex_str.substr(1);
    }

    int r = 0, g = 0, b = 0;
    int a = 255; // Default value if it does not exist in the string

    // Check that the length is valid (6 for RRGGBB or 8 for RRGGBBAA)
    if (hex_str.length() == 6 || hex_str.length() == 8) {
        r = std::stoi(hex_str.substr(0, 2), nullptr, 16);
        g = std::stoi(hex_str.substr(2, 2), nullptr, 16);
        b = std::stoi(hex_str.substr(4, 2), nullptr, 16);

        // If it has 8 characters, the alpha channel is extracted from the last two digits
        if (hex_str.length() == 8) {
            a = std::stoi(hex_str.substr(6, 2), nullptr, 16);
        }
    }

    return { r, g, b, a };
}

/**
 * @brief Converts a vector of flood risk levels to JSON.
 * @param flood_risk_levels The vector of flood risk levels.
 * @return A JSON array containing the flood risk levels.
 */
nlohmann::json ConvertFloodRiskLevelsToJson(const std::vector<config::FloodRiskLevel>& flood_risk_levels) {
    nlohmann::json json_array = nlohmann::json::array();

    for (size_t i = 0; i < flood_risk_levels.size(); ++i) {
        const auto& level = flood_risk_levels[i];
        nlohmann::json item;

        // Format the Label: If the threshold is 0 (e.g., “Dry”), only the name is displayed.
        // If it is greater than that, the format “(X.X m)” is added
        std::string label = level.name;
        if (level.threshold_start > 0.0f) {
            std::ostringstream ss;
            ss << " (" << std::fixed << std::setprecision(3) << level.threshold_start << " m)";
            label += ss.str();
        }

        // Ensure that hexadecimal numbers always start with the ‘#’ prefix
        std::string formato_hex = level.color_hex;
        if (!formato_hex.empty() && formato_hex[0] != '#') {
            formato_hex = "#" + formato_hex;
        }

        // Build the JSON object
        item["label"] = label;
        item["value"] = i;
        item["rgba"] = HexToRgba(level.color_hex);
        item["hex"] = formato_hex;

        // Add to the main array
        json_array.push_back(item);
    }

    return json_array;
}

} // anonymous namespace

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
                                                          const std::string& layer_name,
                                                          const std::vector<config::FloodRiskLevel>& flood_risk_levels) const {
    nlohmann::json payload = {
        {"process", "InitAgent_Layer"},
        {"id", layer_name},
        {"data_path", dataset_name + "/" + layer_name},
        {"data_filename", layer_name},
        {"color_palette", ConvertFloodRiskLevelsToJson(flood_risk_levels)}
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

std::string JsonSerializer::GenerateFrameSyncPayload(sys_time_double time) const {
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
