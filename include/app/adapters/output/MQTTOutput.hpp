/**
 * @file MQTTOutput.hpp
 * @brief Output port adapter for publishing simulation snapshots to an MQTT broker.
 *
 * Handles connecting to an MQTT broker, performing a handshake, and publishing
 * simulation changes incrementally in chunks to optimize network bandwidth.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

#include <mqtt/async_client.h>

#include "app/config/Config.hpp"
#include "app/core/ports/OutputPort.hpp"
#include "app/adapters/output/mqtt/payload/PayloadSerializer.hpp"

namespace floodsim::app::adapters::output {

/**
 * @class MqttOutput
 * @brief Publishes snapshots to an MQTT broker using chunked message payloads.
 */
class MqttOutput : public core::ports::OutputPort {
public:
    /**
     * @brief Constructs a new Mqtt Output instance.
     *
     * @param address The URI of the MQTT broker (e.g., tcp://localhost:1883).
     * @param scenario_name The simulation scenario name, used as the base topic.
     * @param payload_format The serialization format (e.g., JSON).
	 * @param send_initial_state Whether to publish the initial grid state before starting the simulation.
     * @throws std::invalid_argument If address or scenario_name are empty.
     * @throws std::runtime_error If the payload serializer fails to initialize.
     */
    MqttOutput(const std::string& address, const std::string& scenario_name,
        config::OutputConfig::MqttOutputConfigEntry::PayloadFormat payload_format,
        bool send_initial_state);

    /**
     * @brief Destroys the MQTT client, gracefully disconnecting and stopping consumption.
     */
    ~MqttOutput() override;

    /**
     * @brief Main execution loop for the MQTT output thread.
     *
     * Consumes snapshots from the manager and publishes the changes to the broker.
     *
     * @param snapshot_manager Reference to the manager providing simulation snapshots.
     * @param output_path Not actively used by MQTT, but required by interface.
     */
    void Run(core::snapshot::SnapshotManager& snapshot_manager, const std::filesystem::path& output_path) override;

    /**
     * @brief Initializes the output port by publishing the map baseline.
     *
     * @param grid The simulation map grid.
     * @param dataset_name The name of the dataset.
     * @param start_timestamp The initial time of the simulation.
     */
    void SetInitConfig(const core::grid::MapGrid& grid, const std::string& dataset_name,
                       std::chrono::sys_seconds start_timestamp) override;

    /**
     * @brief Retrieves the identifier name for this thread.
     *
     * @return std::string The predefined thread name.
     */
    std::string GetThreadName() const override { return "Out_MQTT"; }

private:
    /**
     * @brief Creates the payload serializer based on the requested format.
     *
     * @param format The requested payload serialization format.
     * @return std::unique_ptr<PayloadSerializer> Instantiated serializer.
     * @throws std::invalid_argument If the format is unknown.
     */
    [[nodiscard]] static std::unique_ptr<PayloadSerializer> CreatePayloadSerializer(
        const config::OutputConfig::MqttOutputConfigEntry::PayloadFormat& format);

    /**
     * @brief Establishes connection to the configured MQTT broker.
     */
    void Connect();

    /**
     * @brief Executes a ping-pong handshake protocol with connected consumers.
     */
    void Handshake();

    /**
     * @brief Sends the initial state of the grid via MQTT.
     *
     * @param grid The initialized map grid.
     * @return GridIndexType The total number of chunks sent.
     */
    GridIndexType SendInitState(const core::grid::MapGrid& grid);

    /**
     * @brief Publishes state changes in a snapshot as MQTT message chunks.
     *
     * @param snapshot The full simulation state.
     * @param changes The differential changes mapped in the current step.
     */
    void PublishInChunks(const core::snapshot::Snapshot& snapshot, const core::snapshot::ChangeList& changes);

    /**
     * @brief Utility to generate an ISO-8601 formatted UTC timestamp.
     *
     * @return std::string Current UTC timestamp.
     */
    std::string GetCurrentTimestamp();

    const int kBatchSize = 10;
    const GridIndexType kChunkSize = 40000;

    const std::string base_topic_;
    std::string client_id_;
    mqtt::async_client client_;

    std::unique_ptr<PayloadSerializer> payload_serializer_;
	bool send_initial_state_;
};

} // namespace floodsim::app::adapters::output
