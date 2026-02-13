
#include "adapters/output/MQTTOutput.hpp"

#include <stdexcept>

#include "adapters/output/mqtt/payload/ProtobufSerializer.hpp"
#include "app/logging/Logger.hpp"

namespace danasim {

    std::unique_ptr<PayloadSerializer> MQTTOutput::createPayloadSerializer(const OutputConfig::MQTTOutputConfigEntry::PayloadFormat& format)
    {
        switch (format) {
        case OutputConfig::MQTTOutputConfigEntry::PayloadFormat::PROTOBUF: {
            LOG_DEBUG("Instantiating Protobuf snapshot serializer.");
            return std::make_unique<ProtobufSerializer>();
        }

        // Future formats (e.g., JSON, FLATBUFFERS) would be added here.

        default: {
            auto msg = fmt::format("PayloadSerializerFactory: Unknown format identifier '{}'",
                static_cast<int>(format));

            LOG_ERROR("{}", msg);
            throw std::invalid_argument(msg);
        }
        }
    }


    MQTTOutput::MQTTOutput(const std::string& address, const std::string& topic,
        const std::string& clientId, int qos, OutputConfig::MQTTOutputConfigEntry::PayloadFormat payloadFormat)
        : topic_(topic)
        , client_(address, clientId)
        , qos_(qos)
        , payloadSerializer_(std::move(createPayloadSerializer(payloadFormat)))
    {
        if (!payloadSerializer_) {
            throw std::runtime_error("MQTTOutput: payload serializer is null");
        }

        connect();
    }

    MQTTOutput::~MQTTOutput() {
        if (client_.is_connected()) {
            client_.disconnect()->wait();
        }
    }

    void MQTTOutput::run(SnapshotManager& snapshotManager, const std::filesystem::path& /* outputPath */)
    {
        StepType lastProcessedStep = -1;

        while (true) {
            try {
                // 1. Bloqueo hasta recibir datos
                // Recibimos el snapshot Y el guardia de seguridad
                auto [data, guard] = snapshotManager.waitForSnapshot(lastProcessedStep);
                auto [snapshot, changes] = data;

                // Si el guard es null, significa que snapshotManager se ha detenido
                if (!guard) {
                    LOG_INFO("Stopping thread (manager stopped).");
                    break;
                }

                LOG_INFO("Consuming snapshot");

                publishInChunks(snapshot, changes);

                LOG_INFO("Publish Completed | Num. Changes: {}", changes.x.size());

                // Actualizamos tracking
                lastProcessedStep = snapshot.step();

                // AL FINAL DEL SCOPE: 'guard' se destruye -> llama a signalDone()
            }
            catch (const std::exception& ex) {
                LOG_ERROR("{}", ex.what());
                // Incluso con error, el loop continúa y el guard liberó al Core.
                // Si quieres que un error mate el hilo, pon un break aquí.
            }
        }
    }

    void MQTTOutput::setGrid(const MapGrid& grid) {

    }

    void MQTTOutput::connect()
    {
        try {
            mqtt::connect_options connOpts;

            connOpts.set_clean_session(true);

            // Permitir más mensajes simultáneos en la red
            connOpts.set_max_inflight(MAX_INFLIGHT);

            client_.connect(connOpts)->wait();

            LOG_INFO("[MQTT] Conectado al broker en {}", client_.get_server_uri());
        }
        catch (const mqtt::exception& exc) {
            LOG_ERROR("[MQTT]: {}", exc.what());
        }
    }

    void MQTTOutput::publishInChunks(const Snapshot& snapshot, const ChangeList& changes) {
        if (changes.x.empty()) return;

        // Define un tamaño de fragmento (ej. 5000 coordenadas por mensaje)
        int32_t totalChunks = static_cast<int32_t>((changes.x.size() + CHUNK_SIZE - 1) / CHUNK_SIZE);

        for (int32_t chunkIndex = 0; chunkIndex < totalChunks; ++chunkIndex) {
            std::string payload;
            payloadSerializer_->serializeChunk(snapshot, changes, chunkIndex, totalChunks, CHUNK_SIZE, payload);

            if (!client_.is_connected()) return;

            try {
                // QoS 0 es rápido (fire and forget), QoS 1 asegura entrega
                mqtt::delivery_token_ptr token = client_.publish(topic_, payload, qos_, false);

                bool last_chunk = (chunkIndex == totalChunks - 1);
                bool must_wait = ((chunkIndex + 1) % BATCH_SIZE == 0);

                if (must_wait || last_chunk) {
                    try {
                        // Esto asegura que el lote actual (y los anteriores) han llegado
                        token->wait();

                        if (token->is_complete()) {
                            LOG_INFO("Publish Chunk {} of {}", (chunkIndex + 1), totalChunks);
                        }
                        else {
                            LOG_ERROR("Message not completed");
                        }
                    }
                    catch (const mqtt::exception& ex) {
                        LOG_ERROR("Error esperando confirmación del lote en chunk {}: {}", chunkIndex, ex.what());
                    }
                }
            }
            catch (const mqtt::exception& exc) {
                LOG_ERROR("Publish: {}", exc.what());
            }
        }
    }

} // namespace danasim
