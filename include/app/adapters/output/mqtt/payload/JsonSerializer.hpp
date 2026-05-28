/**
 * @file JsonSerializer.hpp
 * @brief Concrete implementation of PayloadSerializer for JSON format.
 *
 * This class implements the PayloadSerializer interface, converting
 * simulation states and control messages into JSON-formatted strings
 * using the nlohmann::json library.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <chrono>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "app/adapters/output/mqtt/payload/PayloadSerializer.hpp"

namespace floodsim::app::adapters::output {

/**
 * @class JsonSerializer
 * @brief Serializes and deserializes MQTT payloads into JSON format.
 */
class JsonSerializer final : public PayloadSerializer {
public:
    /**
     * @brief Default constructor.
     */
    JsonSerializer() = default;

    /**
     * @brief Default destructor.
     */
    ~JsonSerializer() override = default;

    /**
     * @brief Generates a JSON-formatted ping payload for the handshake protocol.
     *
     * @param client_id Unique identifier of the core client sending the ping.
     * @param timestamp_utc Current system time encoded as an ISO-8601 UTC string.
     * @return std::string Serialized JSON string containing the ping event.
     */
    std::string GeneratePingPayload(const std::string& client_id,
        const std::string& timestamp_utc) const override;

    /**
     * @brief Parses a raw string payload into a JSON object to extract pong handshake data.
     *
     * @param payload Raw message payload received from the MQTT broker.
     * @return nlohmann::json Parsed JSON object structure representing the pong message.
     * @throws nlohmann::json::exception If the payload string is not a valid JSON structure.
     */
    nlohmann::json ParsePongPayload(const std::string& payload) const override;

    /**
     * @brief Serializes the initial configuration and georeferencing metadata of the map grid.
     *
     * @param grid The simulation map grid containing spatial data, boundaries, and resolution.
     * @param start_timestamp Initial simulation date and time anchor.
     * @return std::string Serialized JSON string containing map configurations and grid setup.
     */
    std::string GenerateInitMapConfigPayload(const core::grid::MapGrid& grid,
        std::chrono::sys_seconds start_timestamp) const override;

    /**
     * @brief Generates the initial descriptor for a specific grid data layer or simulation agent.
     *
     * @param grid The simulation map grid serving as the context.
     * @param dataset_name The name of the active scenario dataset source.
     * @param layer_name The specific layer key identifier (e.g., "topo_bathy").
     * @return std::string Serialized JSON configuration metadata for the specified layer.
     */
    std::string GenerateInitAgentLayerPayload(const std::string& dataset_name,
        const std::string& layer_name) const override;

    /**
     * @brief Generates an End-Of-File (EOF) marker payload for the agent layers initialization sequence.
     *
     * @return std::string Serialized JSON string signaling agent initialization EOF.
     */
    std::string GenerateInitAgentEOFPayload() const override;

    /**
     * @brief Generates the master End-Of-File (EOF) payload completing the entire initialization phase.
     *
     * @param total_chunks Total count of data chunks transmitted during the initialization process.
     * @return std::string Serialized JSON string signaling total initialization completion.
     */
    std::string GenerateInitEOFPayload(GridIndexType total_chunks) const override;

    /**
     * @brief Generates the frame boundary start message payload for a new simulation step.
     *
     * @param total_chunks Total differential cell chunks expected in the current step frame.
     * @param chunks_per_batch Batch processing threshold limit before forcing a flow-control ACK.
     * @return std::string Serialized JSON frame start notification.
     */
    std::string GenerateFrameStartPayload(GridIndexType total_chunks,
        GridIndexType chunks_per_batch) const override;

    /**
     * @brief Serializes a subset batch of simulation cells that changed in value during the step.
     *
     * @param payload_str Output reference where the serialized JSON payload chunk will be written.
     * @param water_depth Reference array containing updated cell water depths.
     * @param flood_risk Reference array containing updated flood hazard classification levels.
     * @param layer_name Context name identifying the reference grid layer.
     * @param changes List mapping the global indices of modified simulation cells.
     * @param init_index The offset index inside the changes collection to begin chunk packaging.
     * @param chunk_size The maximum number of cells to process inside this specific packet block.
     */
    void GenerateChunkChangesPayload(std::string& payload_str,
        const std::vector<float>& water_depth,
        const std::vector<int8_t>& flood_risk,
        const std::string& layer_name,
        const core::snapshot::ChangeList& changes,
        GridIndexType init_index,
        GridIndexType chunk_size) const override;

    /**
     * @brief Decodes a backpressure flow control acknowledgement payload received from consumers.
     *
     * @param payload Raw string payload containing the batch confirmation message.
     * @return nlohmann::json Parsed JSON acknowledgment object structure.
     * @throws nlohmann::json::exception If the incoming payload cannot be parsed into a JSON object.
     */
    nlohmann::json ParseChunkAckPayload(const std::string& payload) const override;

    /**
     * @brief Generates the boundary end message payload closing the current state changes frame.
     *
     * @return std::string Serialized JSON frame end boundary marker.
     */
    std::string GenerateFrameEndPayload() const override;

    /**
     * @brief Generates a synchronization confirmation payload linking physical time metrics.
     *
     * @param time The real/simulated timestamp associated with the completed state frame.
     * @return std::string Serialized JSON time sync packet.
     */
    std::string GenerateFrameSyncPayload(std::chrono::sys_seconds time) const override;

    /**
     * @brief Generates a final terminal event message payload announcing simulation completion.
     *
     * @return std::string Serialized JSON final teardown notification.
     */
    std::string GenerateSimEndPayload() const override;
};

} // namespace floodsim::app::adapters::output
