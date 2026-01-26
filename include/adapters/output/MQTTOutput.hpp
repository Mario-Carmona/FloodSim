
#pragma once

#include <memory>
#include <string>

#include "app/config/Config.hpp"
#include "core/ports/OutputPort.hpp"

#include <mqtt/async_client.h>

namespace danasim {

    // Forward declaration to reduce compile-time dependencies.
    class PayloadSerializer;

    /**
     * @brief Publishes snapshots to an MQTT broker.
     */
    class MQTTOutput : public OutputPort {
    public:
        MQTTOutput(const std::string& address,
            const std::string& topic,
            const std::string& clientId,
            int qos,
            OutputConfig::MQTTOutputConfigEntry::PayloadFormat payloadFormat);

        ~MQTTOutput();

        void run(SnapshotManager& snapshotManager, const std::filesystem::path& outputPath) override;

        std::string getThreadName() const override { return "Out_MQTT"; }

    private:
        [[nodiscard]] // Warns if the returned pointer is ignored (C++17/20)
        static std::unique_ptr<PayloadSerializer> createPayloadSerializer(const OutputConfig::MQTTOutputConfigEntry::PayloadFormat& format);

        void connect();
        void publishInChunks(const Snapshot& snapshot);

        const int MAX_INFLIGHT = 100;
        const int BATCH_SIZE = 20;
        const size_t CHUNK_SIZE = 2000;

        const std::string topic_;
        mqtt::async_client client_;
        int qos_;

        std::unique_ptr<PayloadSerializer> payloadSerializer_;
    };

} // namespace danasim
