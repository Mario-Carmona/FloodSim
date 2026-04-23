
#pragma once

#include <memory>
#include <string>

#include "app/config/Config.hpp"
#include "ports/OutputPort.hpp"

#include <mqtt/async_client.h>

namespace danasim {

    // Forward declaration to reduce compile-time dependencies.
    class PayloadSerializer;

    /**
     * @brief Publishes snapshots to an MQTT broker.
     */
    class MqttOutput : public OutputPort {
    public:
        MqttOutput(const std::string& address,
            const std::string& scenarioName,
            OutputConfig::MqttOutputConfigEntry::PayloadFormat payloadFormat);

        ~MqttOutput();

        void run(SnapshotManager& snapshotManager, const std::filesystem::path& outputPath) override;

        void setInitConfig(const MapGrid& grid, const std::string& datasetName, std::chrono::sys_seconds startTimestamp) override;

        std::string getThreadName() const override { return "Out_MQTT"; }

    private:
        [[nodiscard]] // Warns if the returned pointer is ignored (C++17/20)
        static std::unique_ptr<PayloadSerializer> createPayloadSerializer(const OutputConfig::MqttOutputConfigEntry::PayloadFormat& format);

        void connect();
        void handshake();

        GridIndexType sendInitState(const MapGrid& grid);

        void publishInChunks(const Snapshot& snapshot, const ChangeList& changes);

        std::string getCurrentTimestampUTC();

        const int BATCH_SIZE = 40;
        const GridIndexType CHUNK_SIZE = 4000;

        const std::string baseTopic_;
        std::string clientID_;
        mqtt::async_client client_;

        std::unique_ptr<PayloadSerializer> payloadSerializer_;
    };

} // namespace danasim
