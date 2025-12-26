
#include "adapters/output/MQTTOutput.hpp"

#include <stdexcept>

#include "app/logging/Logger.hpp"

namespace danasim {

    MQTTOutput::MQTTOutput(const std::string& address, const std::string& topic,
        const std::string& clientId, int qos, std::unique_ptr<SnapshotSerializerPort> serializer)
        : topic_(topic)
        , client_(address, clientId)
        , qos_(qos)
        , serializer_(std::move(serializer))
    {
        if (!serializer_) {
            throw std::runtime_error("MQTTOutput: serializer is null");
        }

        connect();
    }

    MQTTOutput::~MQTTOutput() {
        if (client_.is_connected()) {
            client_.disconnect()->wait();
        }
    }

    void MQTTOutput::run(SnapshotManager& snapshotManager)
    {
        uint64_t lastProcessedStep = 0xFFFFFFFFFFFFFFFF;

        while (true) {
            try {
                // 1. Bloqueo hasta recibir datos
                // Recibimos el snapshot Y el guardia de seguridad
                auto [snapshot, guard] = snapshotManager.waitForSnapshot(lastProcessedStep);

                // Si el guard es null, significa que snapshotManager se ha detenido
                if (!guard) {
                    LOG_INFO("[MQTTOutput] Stopping thread (manager stopped).");
                    break;
                }

                LOG_INFO("[MQTTOutput]: Consuming");

                // 2. Procesamiento
                // Si esto falla (excepción), 'guard' se destruye aquí y avisa al core automáticamente.
                std::string payload;
                serializer_->serializeToStr(snapshot, payload);
                publish(payload);

                // Actualizamos tracking
                lastProcessedStep = snapshot.step();

                // AL FINAL DEL SCOPE: 'guard' se destruye -> llama a signalDone()
            }
            catch (const std::exception& ex) {
                LOG_ERROR("[MQTTOutput Error]: {}", ex.what());
                // Incluso con error, el loop continúa y el guard liberó al Core.
                // Si quieres que un error mate el hilo, pon un break aquí.
            }
        }
    }

    void MQTTOutput::connect()
    {
        try {
            mqtt::connect_options connOpts;
            connOpts.set_clean_session(true);
            client_.connect(connOpts)->wait();
            LOG_INFO("[MQTT] Conectado al broker en {}", client_.get_server_uri());
        }
        catch (const mqtt::exception& exc) {
            LOG_ERROR("[MQTT]: {}", exc.what());
        }
    }

    void MQTTOutput::publish(const std::string& payload)
    {
        if (!client_.is_connected()) return;

        try {
            LOG_INFO("[MQTT Publish]: Done");
            // QoS 0 es rápido (fire and forget), QoS 1 asegura entrega
            client_.publish(topic_, payload, qos_, false);
        }
        catch (const mqtt::exception& exc) {
            LOG_ERROR("[MQTT Publish]: {}", exc.what());
        }
    }

} // namespace danasim
