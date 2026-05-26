/**
 * @file MQTTOutput.cpp
 * @brief Implementation of the MqttOutput class for handling MQTT data publishing.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "app/adapters/output/MQTTOutput.hpp"

#include <algorithm>
#include <stdexcept>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include "app/adapters/output/mqtt/payload/JsonSerializer.hpp"
#include "logging/Logger.hpp"

namespace floodsim {

std::unique_ptr<PayloadSerializer> MqttOutput::CreatePayloadSerializer(
        const OutputConfig::MqttOutputConfigEntry::PayloadFormat& format) {

    switch (format) {
    case OutputConfig::MqttOutputConfigEntry::PayloadFormat::kJson: {
        LOG_DEBUG("Instantiating JSON snapshot serializer.");
        return std::make_unique<JsonSerializer>();
    }
                                                                    // Future formats (e.g., FLATBUFFERS, PROTOBUF) would be added here.
    default: {
        auto msg = fmt::format("PayloadSerializerFactory: Unknown format identifier '{}'",
            static_cast<int>(format));
        LOG_ERROR("{}", msg);
        throw std::invalid_argument(msg);
    }
    }
}

MqttOutput::MqttOutput(const std::string& address, const std::string& scenario_name,
                       OutputConfig::MqttOutputConfigEntry::PayloadFormat payload_format)
    : base_topic_("FloodSim/" + scenario_name)
    , client_id_("FloodSim_Core")
    , client_(address, client_id_)
    , payload_serializer_(std::move(CreatePayloadSerializer(payload_format))) {

    if (address.empty() || scenario_name.empty()) {
        throw std::invalid_argument("MqttOutput: Address and scenario_name cannot be empty.");
    }

    if (!payload_serializer_) {
        throw std::runtime_error("MqttOutput: payload_serializer_ is null.");
    }

    Connect();
}

MqttOutput::~MqttOutput() {
    if (client_.is_connected()) {
        const std::string topic_end(base_topic_ + "/events");

        std::string sim_end_payload = payload_serializer_->GenerateSimEndPayload();

        auto sim_end_msg = mqtt::make_message(topic_end, sim_end_payload);
        sim_end_msg->set_qos(1);
        client_.publish(sim_end_msg)->wait();

        client_.stop_consuming();
        client_.disconnect()->wait();
    }
}

void MqttOutput::Connect() {
    try {
        mqtt::connect_options conn_opts;
        conn_opts.set_clean_session(true);

        // Allow more simultaneous messages on the network if required in the future
        // conn_opts.set_max_inflight(MAX_INFLIGHT);

        client_.connect(conn_opts)->wait();
        LOG_INFO("[MQTT] Connected to broker at {}", client_.get_server_uri());

        client_.start_consuming();
        Handshake();
    }
    catch (const mqtt::exception& exc) {
        LOG_ERROR("[MQTT] Connection Error: {}", exc.what());
    }
}

void MqttOutput::Handshake() {
    const std::string topic_ping(base_topic_ + "/system/handshake/ping");
    const std::string topic_pong(base_topic_ + "/system/handshake/pong");

    const std::chrono::milliseconds ping_interval(10000); // Retry every 10 seconds

    client_.subscribe(topic_pong, 1)->wait();

    mqtt::const_message_ptr msg;
    bool pong_received = false;
    LOG_INFO("[MQTT] Initiating handshake...");

    while (!pong_received) {
        std::string ping_payload = payload_serializer_->GeneratePingPayload(client_id_, GetCurrentTimestampUTC());

        auto pub_msg = mqtt::make_message(topic_ping, ping_payload);
        pub_msg->set_qos(1);

        try {
            client_.publish(pub_msg)->wait();
            LOG_DEBUG("[MQTT] Ping sent, waiting for Pong...");
        }
        catch (const mqtt::exception& e) {
            LOG_ERROR("[MQTT] Error publishing Ping: {}", e.what());
        }

        if (client_.try_consume_message_for(&msg, ping_interval)) {
            if (msg && msg->get_topic_ref().to_string() == topic_pong) {
                try {
                    nlohmann::json pong_payload = payload_serializer_->ParsePongPayload(msg->get_payload_ref().to_string());

                    if (pong_payload.contains("process") && pong_payload["process"] == "System_Pong") {
                        LOG_INFO("[MQTT] Pong received from: {}", pong_payload["source"].get<std::string>());
                        pong_received = true;
                    }
                }
                catch (const nlohmann::json::exception& e) {
                    LOG_ERROR("[MQTT] Malformed JSON in Pong topic: {}", e.what());
                }
            }
        }
        else {
            LOG_WARN("[MQTT] Handshake timeout. Retrying ping...");
        }
    }

    client_.unsubscribe(topic_pong)->wait();
}

void MqttOutput::SetInitConfig(const MapGrid& grid, const std::string& dataset_name,
                               std::chrono::sys_seconds start_timestamp) {

    const std::string topic_init(base_topic_ + "/events");

    std::string init_map_config_payload = payload_serializer_->GenerateInitMapConfigPayload(grid, start_timestamp);
    auto init_map_config_msg = mqtt::make_message(topic_init, init_map_config_payload);
    init_map_config_msg->set_qos(1);
    client_.publish(init_map_config_msg)->wait();

    std::string init_agent_layer_payload = payload_serializer_->GenerateInitAgentLayerPayload(grid, dataset_name, "topo_bathy");
    auto init_agent_layer_msg = mqtt::make_message(topic_init, init_agent_layer_payload);
    init_agent_layer_msg->set_qos(1);
    client_.publish(init_agent_layer_msg)->wait();

    std::string init_agent_eof_payload = payload_serializer_->GenerateInitAgentEOFPayload();
    auto init_agent_eof_msg = mqtt::make_message(topic_init, init_agent_eof_payload);
    init_agent_eof_msg->set_qos(1);
    client_.publish(init_agent_eof_msg)->wait();

    GridIndexType total_chunks = SendInitState(grid);

    std::string init_eof_payload = payload_serializer_->GenerateInitEOFPayload(total_chunks);
    auto init_eof_msg = mqtt::make_message(topic_init, init_eof_payload);
    init_eof_msg->set_qos(1);
    client_.publish(init_eof_msg)->wait();
}

GridIndexType MqttOutput::SendInitState(const MapGrid& grid) {
    const std::string topic_changes(base_topic_ + "/events");
    const std::string topic_control_changes(base_topic_ + "/control/events");

    auto total_start = std::chrono::high_resolution_clock::now();

    client_.subscribe(topic_control_changes, 1)->wait();

    std::string payload;

    const std::vector<float>& water_depth = grid.GetLayer<float>("water_depth")->GetData();
    const std::vector<int8_t>& flood_risk = grid.GetLayer<int8_t>("flood_risk")->GetData();

    ChangeList changes;
    for (size_t i = 0; i < flood_risk.size(); ++i) {
        if (flood_risk[i] > 0) {
            changes.indexes.push_back(i);
        }
    }

    int chunk_count = 0;

    LOG_INFO("[MQTT] Starting async initial state transmission ({} cells)...", changes.indexes.size());

    GridIndexType total_chunks = (static_cast<GridIndexType>(changes.indexes.size()) + kChunkSize - 1) / kChunkSize;

    std::string frame_start_payload = payload_serializer_->GenerateFrameStartPayload(total_chunks, kBatchSize);
    auto frame_start_msg = mqtt::make_message(topic_changes, frame_start_payload);
    frame_start_msg->set_qos(1);
    client_.publish(frame_start_msg)->wait();

    GridIndexType init_index = 0;

    while (init_index < changes.indexes.size()) {
        GridIndexType current_chunk_size = std::min(kChunkSize, static_cast<GridIndexType>(changes.indexes.size() - init_index));

        payload_serializer_->GenerateChunkChangesPayload(payload, water_depth, flood_risk, "topo_bathy", changes, init_index, current_chunk_size);

        init_index += current_chunk_size;

        auto msg = mqtt::make_message(topic_changes, payload);
        msg->set_qos(1);

        try {
            // Async sending: runs in background returning a token immediately
            client_.publish(msg);
            chunk_count++;

            // --- FLOW CONTROL (Backpressure) ---
            // If the batch limit is reached, wait for Mosquitto to confirm reception.
            if (chunk_count % kBatchSize == 0) {
                auto start = std::chrono::high_resolution_clock::now();
                auto msg_ack = client_.consume_message();
                auto end = std::chrono::high_resolution_clock::now();

                std::chrono::duration<double> elapsed = end - start;
                LOG_DEBUG("Wait Time: {}s", elapsed.count());

                if (msg_ack && msg_ack->get_topic_ref().to_string() == topic_control_changes) {
                    nlohmann::json chunk_ack_payload = payload_serializer_->ParseChunkAckPayload(msg_ack->get_payload_ref().to_string());

                    if (chunk_ack_payload.contains("process") && chunk_ack_payload["process"] == "ChunkAck") {
                        LOG_DEBUG("[MQTT] Chunk Ack received");
                    }
                }
                LOG_DEBUG("[MQTT] Batch confirmed. Chunks {}. Current index {}", chunk_count, init_index);
            }
        }
        catch (const mqtt::exception& e) {
            LOG_ERROR("[MQTT] Error publishing initial state: {}", e.what());
        }
    }

    std::string frame_end_payload = payload_serializer_->GenerateFrameEndPayload();
    auto frame_end_msg = mqtt::make_message(topic_changes, frame_end_payload);
    frame_end_msg->set_qos(1);
    client_.publish(frame_end_msg)->wait();

    LOG_INFO("[MQTT] Initial state transmission completed. Total chunks: {}", chunk_count);

    client_.unsubscribe(topic_control_changes)->wait();

    auto total_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = total_end - total_start;
    LOG_DEBUG("Total Time: {}s", elapsed.count());

    return chunk_count;
}

void MqttOutput::Run(SnapshotManager& snapshot_manager, const std::filesystem::path& /* output_path */) {
    std::chrono::system_clock::time_point last_processed_step = std::chrono::system_clock::time_point::min();

    while (true) {
        try {
            // 1. Block until data is received
            auto [data, guard] = snapshot_manager.WaitForSnapshot(last_processed_step);
            auto [snapshot, changes] = data;

            // If guard is null, snapshot_manager was stopped
            if (!guard) {
                LOG_INFO("Stopping MQTT thread (manager stopped).");
                break;
            }

            LOG_INFO("Consuming snapshot");

            PublishInChunks(snapshot, changes);

            LOG_INFO("Publish Completed | Num. Changes: {}", changes.indexes.size());

            // Update tracking
            last_processed_step = snapshot.Time();

            // END OF SCOPE: 'guard' is destroyed, signaling done
        }
        catch (const std::exception& ex) {
            LOG_ERROR("{}", ex.what());
            // Execution continues gracefully
        }
    }
}

void MqttOutput::PublishInChunks(const Snapshot& snapshot, const ChangeList& changes) {
    const std::string topic_changes(base_topic_ + "/events");
    const std::string topic_control_changes(base_topic_ + "/control/events");

    client_.subscribe(topic_control_changes, 1)->wait();

    std::string payload;

    GridIndexType total_chunks = (static_cast<GridIndexType>(changes.indexes.size()) + kChunkSize - 1) / kChunkSize;

    std::string frame_start_payload = payload_serializer_->GenerateFrameStartPayload(total_chunks, kBatchSize);
    auto frame_start_msg = mqtt::make_message(topic_changes, frame_start_payload);
    frame_start_msg->set_qos(1);
    client_.publish(frame_start_msg)->wait();

    int chunk_count = 0;

    LOG_INFO("[MQTT] Starting async chunk transmission ({} cells)...", changes.indexes.size());

    GridIndexType init_index = 0;

    while (init_index < changes.indexes.size()) {
        GridIndexType current_chunk_size = std::min(kChunkSize, static_cast<GridIndexType>(changes.indexes.size() - init_index));

        payload_serializer_->GenerateChunkChangesPayload(payload, snapshot.WaterDepth(), snapshot.FloodRisk(), "topo_bathy", changes, init_index, current_chunk_size);

        init_index += current_chunk_size;

        auto msg = mqtt::make_message(topic_changes, payload);
        msg->set_qos(1);

        try {
            client_.publish(msg);
            chunk_count++;

            // --- FLOW CONTROL (Backpressure) ---
            if (chunk_count % kBatchSize == 0) {
                auto msg_ack = client_.consume_message();

                if (msg_ack && msg_ack->get_topic_ref().to_string() == topic_control_changes) {
                    nlohmann::json chunk_ack_payload = payload_serializer_->ParseChunkAckPayload(msg_ack->get_payload_ref().to_string());

                    if (chunk_ack_payload.contains("process") && chunk_ack_payload["process"] == "ChunkAck") {
                        LOG_DEBUG("[MQTT] Chunk Ack received");
                    }
                }
                LOG_DEBUG("[MQTT] Batch confirmed. Chunks {}. Current index {}", chunk_count, init_index);
            }
        }
        catch (const mqtt::exception& e) {
            LOG_ERROR("[MQTT] Error publishing state chunk: {}", e.what());
        }
    }

    std::string frame_end_payload = payload_serializer_->GenerateFrameEndPayload();
    auto frame_end_msg = mqtt::make_message(topic_changes, frame_end_payload);
    frame_end_msg->set_qos(1);
    client_.publish(frame_end_msg)->wait();

    std::string frame_sync_payload = payload_serializer_->GenerateFrameSyncPayload(snapshot.Time());
    auto frame_sync_msg = mqtt::make_message(topic_changes, frame_sync_payload);
    frame_sync_msg->set_qos(1);
    client_.publish(frame_sync_msg)->wait();

    client_.unsubscribe(topic_control_changes)->wait();
}

std::string MqttOutput::GetCurrentTimestampUTC() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm local_time = *std::localtime(&in_time_t);

    return std::string(fmt::format("{:%Y-%m-%dT%H:%M:%S}", local_time));
}

} // namespace floodsim
