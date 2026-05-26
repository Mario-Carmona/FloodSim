/**
 * @file PayloadSerializer.hpp
 * @brief Interface for serializing and deserializing MQTT payloads.
 *
 * Defines the contract for converting simulation state data, control messages,
 * and grid structures into transport-agnostic string payloads (e.g., JSON, Protobuf).
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "app/core/grid/MapGrid.hpp"
#include "app/core/snapshot/ChangeList.hpp"
#include "app/core/snapshot/Snapshot.hpp"

namespace floodsim::app::adapters::output {

/**
 * @class PayloadSerializer
 * @brief Abstract base class for MQTT payload serializers.
 */
class PayloadSerializer {
public:
    PayloadSerializer() = default;

    virtual ~PayloadSerializer() = default;

    /**
     * @brief Generates a ping payload for MQTT connection handshake.
     *
     * @param client_id The unique identifier of the MQTT client.
     * @param timestamp_utc The current UTC timestamp string.
     * @return std::string The serialized ping payload.
     */
    virtual std::string GeneratePingPayload(const std::string& client_id,
        const std::string& timestamp_utc) const = 0;

    /**
     * @brief Parses a pong payload received during the handshake.
     *
     * @param payload The raw string payload received from the broker.
     * @return nlohmann::json The parsed JSON representation of the pong message.
     * @throws nlohmann::json::exception If the payload is malformed.
     */
    virtual nlohmann::json ParsePongPayload(const std::string& payload) const = 0;

    /**
     * @brief Generates the initial configuration payload for the map grid.
     *
     * @param grid The simulation map grid.
     * @param start_timestamp The initial time of the simulation.
     * @return std::string The serialized map configuration payload.
     */
    virtual std::string GenerateInitMapConfigPayload(const core::grid::MapGrid& grid,
        std::chrono::sys_seconds start_timestamp) const = 0;

    /**
     * @brief Generates the initial payload for a specific agent/layer in the grid.
     *
     * @param grid The simulation map grid.
     * @param dataset_name The name of the dataset.
     * @param layer_name The name of the specific layer to serialize (e.g., "topo_bathy").
     * @return std::string The serialized agent layer payload.
     */
    virtual std::string GenerateInitAgentLayerPayload(const core::grid::MapGrid& grid,
        const std::string& dataset_name,
        const std::string& layer_name) const = 0;

    /**
     * @brief Generates the End-Of-File (EOF) marker payload for the initialization agents.
     *
     * @return std::string The serialized init agent EOF payload.
     */
    virtual std::string GenerateInitAgentEOFPayload() const = 0;

    /**
     * @brief Generates the EOF marker payload for the entire initialization sequence.
     *
     * @param total_chunks The total number of chunks that were sent.
     * @return std::string The serialized init EOF payload.
     */
    virtual std::string GenerateInitEOFPayload(GridIndexType total_chunks) const = 0;

    /**
     * @brief Generates the start payload for a new simulation frame.
     *
     * @param total_chunks The total number of state chunks to expect in this frame.
     * @param chunks_per_batch The number of chunks sent before expecting an ACK.
     * @return std::string The serialized frame start payload.
     */
    virtual std::string GenerateFrameStartPayload(GridIndexType total_chunks,
        GridIndexType chunks_per_batch) const = 0;

    /**
     * @brief Generates a chunk payload containing subset changes for the current simulation step.
     *
     * @param payload_str Output parameter where the generated string will be written.
     * @param water_depth Reference to the water depth data array.
     * @param flood_risk Reference to the flood risk classification array.
     * @param layer_name The identifier of the reference layer.
     * @param changes The list of active cell indices that have changed.
     * @param init_index The starting index within the changes array for this chunk.
     * @param chunk_size The number of cells to process in this specific chunk.
     */
    virtual void GenerateChunkChangesPayload(std::string& payload_str,
        const std::vector<float>& water_depth,
        const std::vector<int8_t>& flood_risk,
        const std::string& layer_name,
        const core::snapshot::ChangeList& changes,
        GridIndexType init_index,
        GridIndexType chunk_size) const = 0;

    /**
     * @brief Parses an acknowledgement (ACK) payload for a received chunk batch.
     *
     * @param payload The raw string payload containing the ACK.
     * @return nlohmann::json The parsed JSON structure of the ACK.
     * @throws nlohmann::json::exception If the payload is malformed.
     */
    virtual nlohmann::json ParseChunkAckPayload(const std::string& payload) const = 0;

    /**
     * @brief Generates the end payload for a simulation frame.
     *
     * @return std::string The serialized frame end payload.
     */
    virtual std::string GenerateFrameEndPayload() const = 0;

    /**
     * @brief Generates a synchronization payload marking the time completion of a frame.
     *
     * @param time The timestamp of the completed frame.
     * @return std::string The serialized frame sync payload.
     */
    virtual std::string GenerateFrameSyncPayload(std::chrono::sys_seconds time) const = 0;

    /**
     * @brief Generates the termination payload indicating the simulation has ended.
     *
     * @return std::string The serialized simulation end payload.
     */
    virtual std::string GenerateSimEndPayload() const = 0;
};

} // namespace floodsim::app::adapters::output
