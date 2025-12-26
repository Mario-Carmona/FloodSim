
#pragma once

#include <memory>
#include <string>

#include "core/ports/OutputPort.hpp"
#include "core/ports/SnapshotSerializerPort.hpp"

#include <mqtt/async_client.h>

namespace danasim {

    /**
     * @brief Publishes snapshots to an MQTT broker.
     */
    class MQTTOutput : public OutputPort {
    public:
        MQTTOutput(const std::string& address,
            const std::string& topic,
            const std::string& clientId,
            int qos,
            std::unique_ptr<SnapshotSerializerPort> serializer);

        ~MQTTOutput();

        void run(SnapshotManager& snapshotManager) override;

        std::string getThreadName() const override { return "Out_MQTT"; }

    private:
        void connect();
        void publish(const std::string& payload);

        const std::string topic_;
        mqtt::async_client client_;
        int qos_;

        std::unique_ptr<SnapshotSerializerPort> serializer_;
    };

} // namespace danasim
