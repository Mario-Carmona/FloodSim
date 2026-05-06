
#include "app/adapters/output/MQTTOutput.hpp"

#include <stdexcept>

#include <nlohmann/json.hpp>

#include <fmt/core.h>
#include <fmt/chrono.h>

#include "app/adapters/output/mqtt/payload/JsonSerializer.hpp"
#include "logging/Logger.hpp"
#include "app/core/common/SimulationConstants.hpp"

namespace danasim {

    std::unique_ptr<PayloadSerializer> MqttOutput::createPayloadSerializer(const OutputConfig::MqttOutputConfigEntry::PayloadFormat& format)
    {
        switch (format) {
        case OutputConfig::MqttOutputConfigEntry::PayloadFormat::JSON: {
            LOG_DEBUG("Instantiating Protobuf snapshot serializer.");
            return std::make_unique<JsonSerializer>();
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


    MqttOutput::MqttOutput(const std::string& address, const std::string& scenarioName, OutputConfig::MqttOutputConfigEntry::PayloadFormat payloadFormat)
        : baseTopic_("FloodSim/" + scenarioName)
        , clientID_("FloodSim_Core")
        , client_(address, clientID_)
        , payloadSerializer_(std::move(createPayloadSerializer(payloadFormat)))
    {
        if (!payloadSerializer_) {
            throw std::runtime_error("MqttOutput: payload serializer is null");
        }

        connect();
    }

    MqttOutput::~MqttOutput() {
        if (client_.is_connected()) {
            const std::string TOPIC_END(baseTopic_ + "/events");

            std::string simEndPayload = payloadSerializer_->generateSimEndPayload();

            auto simEndMsg = mqtt::make_message(TOPIC_END, simEndPayload);
            simEndMsg->set_qos(1);
            client_.publish(simEndMsg)->wait();


            client_.stop_consuming();

            client_.disconnect()->wait();
        }
    }

    void MqttOutput::connect()
    {
        try {
            mqtt::connect_options connOpts;

            connOpts.set_clean_session(true);

            // Permitir más mensajes simultáneos en la red
            // connOpts.set_max_inflight(MAX_INFLIGHT);

            client_.connect(connOpts)->wait();

            LOG_INFO("[MQTT] Conectado al broker en {}", client_.get_server_uri());

            client_.start_consuming();
            
            handshake();
        }
        catch (const mqtt::exception& exc) {
            LOG_ERROR("[MQTT]: {}", exc.what());
        }
    } 

    void MqttOutput::handshake() {
        const std::string TOPIC_PING(baseTopic_ + "/system/handshake/ping");
        const std::string TOPIC_PONG(baseTopic_ + "/system/handshake/pong");

        const std::chrono::milliseconds PING_INTERVAL(10000); // Reintentar cada 10 segundos


        client_.subscribe(TOPIC_PONG, 1)->wait();


        mqtt::const_message_ptr msg;
        bool pong_received = false;
        LOG_INFO("[MQTT] Iniciando handshake...");

        while (!pong_received) {
            std::string ping_payload = payloadSerializer_->generatePingPayload(clientID_, getCurrentTimestampUTC());

            auto pubmsg = mqtt::make_message(TOPIC_PING, ping_payload);
            pubmsg->set_qos(1);

            try {
                client_.publish(pubmsg)->wait();
                LOG_DEBUG("[MQTT] Ping enviado, esperando Pong...");
            }
            catch (const mqtt::exception& e) {
                LOG_ERROR("[MQTT] Error al publicar Ping: {}", e.what());
            }


            if (client_.try_consume_message_for(&msg, PING_INTERVAL)) {
                if (msg && msg->get_topic_ref().to_string() == TOPIC_PONG) {
                    try {
                        nlohmann::json pong_payload = payloadSerializer_->parsePongPayload(msg->get_payload_ref().to_string());

                        if (pong_payload.contains("process") && pong_payload["process"] == "System_Pong") {
                            LOG_INFO("[MQTT] Pong recibido de: {}", pong_payload["source"].get<std::string>());
                            pong_received = true;
                        }
                    }
                    catch (const nlohmann::json::exception& e) {
                        LOG_ERROR("[MQTT] JSON mal formado en topic Pong: {}", e.what());
                    }
                }
            }
            else {
                LOG_WARN("[MQTT] Timeout de handshake. Reintentando ping...");
            }
        }

        
        client_.unsubscribe(TOPIC_PONG)->wait();
    }

    void MqttOutput::setInitConfig(const MapGrid& grid, const std::string& datasetName, std::chrono::sys_seconds startTimestamp) {
        const std::string TOPIC_INIT(baseTopic_ + "/events");
        

        std::string initMapConfigPayload = payloadSerializer_->generateInitMapConfigPayload(grid, startTimestamp);

        auto initMapConfigMsg = mqtt::make_message(TOPIC_INIT, initMapConfigPayload);
        initMapConfigMsg->set_qos(1);
        client_.publish(initMapConfigMsg)->wait();


        std::string initAgentLayerPayload = payloadSerializer_->generateInitAgentLayerPayload(grid, datasetName, "topo_bathy");

        auto initAgentLayerMsg = mqtt::make_message(TOPIC_INIT, initAgentLayerPayload);
        initAgentLayerMsg->set_qos(1);
        client_.publish(initAgentLayerMsg)->wait();


        std::string initAgentEOFPayload = payloadSerializer_->generateInitAgentEOFPayload();

        auto initAgentEOFMsg = mqtt::make_message(TOPIC_INIT, initAgentEOFPayload);
        initAgentEOFMsg->set_qos(1);
        client_.publish(initAgentEOFMsg)->wait();


        GridIndexType totalChunks = sendInitState(grid);


        std::string initEOFPayload = payloadSerializer_->generateInitEOFPayload(totalChunks);

        auto initEOFMsg = mqtt::make_message(TOPIC_INIT, initEOFPayload);
        initEOFMsg->set_qos(1);
        client_.publish(initEOFMsg)->wait();
    }

    GridIndexType MqttOutput::sendInitState(const MapGrid& grid) {
        const std::string TOPIC_CHANGES(baseTopic_ + "/events");
        const std::string TOPIC_CONTROL_CHANGES(baseTopic_ + "/control/events");

        auto totalStart = std::chrono::high_resolution_clock::now();

        client_.subscribe(TOPIC_CONTROL_CHANGES, 1)->wait();


        std::string payload;


        const std::vector<float>& water_depth = grid.getLayer<float>("water_depth")->getData();
        const std::vector<int8_t>& flood_risk = grid.getLayer<int8_t>("flood_risk")->getData();

        ChangeList changes;
        for (size_t i = 0; i < water_depth.size(); ++i) {
            if (water_depth[i] > DRY_TOLERANCE) {
                changes.indexes.push_back(i);
            }
        }


        int chunk_count = 0;

        LOG_INFO("[MQTT] Iniciando envío asíncrono del estado inicial ({} celdas)...", changes.indexes.size());


		GridIndexType totalChunks = (static_cast<GridIndexType>(changes.indexes.size()) + CHUNK_SIZE - 1) / CHUNK_SIZE; // Redondeo hacia arriba

        std::string frameStartPayload = payloadSerializer_->generateFrameStartPayload(totalChunks, BATCH_SIZE);

        auto frameStartMsg = mqtt::make_message(TOPIC_CHANGES, frameStartPayload);
        frameStartMsg->set_qos(1);
        client_.publish(frameStartMsg)->wait();


        GridIndexType initIndex = 0;

        while (initIndex < changes.indexes.size()) {
            GridIndexType currentChunkSize = std::min(CHUNK_SIZE, static_cast<GridIndexType>(changes.indexes.size() - initIndex));

            payloadSerializer_->generateChunkChangesPayload(payload, water_depth, flood_risk, "topo_bathy", changes, initIndex, currentChunkSize);

            initIndex += currentChunkSize;

            auto msg = mqtt::make_message(TOPIC_CHANGES, payload);
            msg->set_qos(1);

            try {
                // Envío asíncrono: se ejecuta en segundo plano y devuelve un token inmediatamente
                client_.publish(msg);

                chunk_count++;

                // --- CONTROL DE FLUJO (Backpressure) ---
                // Si hemos alcanzado el límite del lote, esperamos a que Mosquitto confirme 
                // la recepción de todos antes de saturar los buffers.
                if (chunk_count % BATCH_SIZE == 0) {
                    auto start = std::chrono::high_resolution_clock::now();

                    auto msg_ack = client_.consume_message();

                    auto end = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double> elapsed = end - start;
                    LOG_DEBUG("Tiempo Esperando: {}s", elapsed.count());

                    if (msg_ack && msg_ack->get_topic_ref().to_string() == TOPIC_CONTROL_CHANGES) {
                        nlohmann::json chunk_ack_payload = payloadSerializer_->parseChunkAckPayload(msg_ack->get_payload_ref().to_string());

                        if (chunk_ack_payload.contains("process") && chunk_ack_payload["process"] == "ChunkAck") {
                            LOG_INFO("[MQTT] Chunk Ack recibido");
                        }
                    }


                    LOG_DEBUG("[MQTT] Lote confirmado. Chunks {}. Current index {}", chunk_count, initIndex);
                }
            }
            catch (const mqtt::exception& e) {
                LOG_ERROR("[MQTT] Error al publicar estado inicial: {}", e.what());
            }
        }

        

        std::string frameEndPayload = payloadSerializer_->generateFrameEndPayload();

        auto frameEndMsg = mqtt::make_message(TOPIC_CHANGES, frameEndPayload);
        frameEndMsg->set_qos(1);
        client_.publish(frameEndMsg)->wait();



        LOG_INFO("[MQTT] Envío de estado inicial completado. Total chunks: {}", chunk_count);

        client_.unsubscribe(TOPIC_CONTROL_CHANGES)->wait();

        auto totalEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = totalEnd - totalStart;
        LOG_DEBUG("Tiempo Total: {}s", elapsed.count());

        return chunk_count;
    }

    void MqttOutput::run(SnapshotManager& snapshotManager, const std::filesystem::path& /* outputPath */)
    {
        std::chrono::system_clock::time_point lastProcessedStep = std::chrono::system_clock::time_point::min();

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

                LOG_INFO("Publish Completed | Num. Changes: {}", changes.indexes.size());

                // Actualizamos tracking
                lastProcessedStep = snapshot.time();

                // AL FINAL DEL SCOPE: 'guard' se destruye -> llama a signalDone()
            }
            catch (const std::exception& ex) {
                LOG_ERROR("{}", ex.what());
                // Incluso con error, el loop continúa y el guard liberó al Core.
                // Si quieres que un error mate el hilo, pon un break aquí.
            }
        }
    }

    void MqttOutput::publishInChunks(const Snapshot& snapshot, const ChangeList& changes) {
        const std::string TOPIC_CHANGES(baseTopic_ + "/events");
        const std::string TOPIC_CONTROL_CHANGES(baseTopic_ + "/control/events");


        client_.subscribe(TOPIC_CONTROL_CHANGES, 1)->wait();


        std::string payload;



        GridIndexType totalChunks = (static_cast<GridIndexType>(changes.indexes.size()) + CHUNK_SIZE - 1) / CHUNK_SIZE; // Redondeo hacia arriba

        std::string frameStartPayload = payloadSerializer_->generateFrameStartPayload(totalChunks, BATCH_SIZE);

        auto frameStartMsg = mqtt::make_message(TOPIC_CHANGES, frameStartPayload);
        frameStartMsg->set_qos(1);
        client_.publish(frameStartMsg)->wait();


        int chunk_count = 0;

        LOG_INFO("[MQTT] Iniciando envío asíncrono del estado inicial ({} celdas)...", changes.indexes.size());

        GridIndexType initIndex = 0;

        while (initIndex < changes.indexes.size()) {
            GridIndexType currentChunkSize = std::min(CHUNK_SIZE, static_cast<GridIndexType>(changes.indexes.size() - initIndex));

            payloadSerializer_->generateChunkChangesPayload(payload, snapshot.waterDepth(), snapshot.floodRisk(), "topo_bathy", changes, initIndex, currentChunkSize);

            initIndex += currentChunkSize;

            auto msg = mqtt::make_message(TOPIC_CHANGES, payload);
            msg->set_qos(1);

            try {
                // Envío asíncrono: se ejecuta en segundo plano y devuelve un token inmediatamente
                client_.publish(msg);

                chunk_count++;

                // --- CONTROL DE FLUJO (Backpressure) ---
                // Si hemos alcanzado el límite del lote, esperamos a que Mosquitto confirme 
                // la recepción de todos antes de saturar los buffers.
                if (chunk_count % BATCH_SIZE == 0) {
                    auto msg_ack = client_.consume_message();

                    if (msg_ack && msg_ack->get_topic_ref().to_string() == TOPIC_CONTROL_CHANGES) {
                        nlohmann::json chunk_ack_payload = payloadSerializer_->parseChunkAckPayload(msg_ack->get_payload_ref().to_string());

                        if (chunk_ack_payload.contains("process") && chunk_ack_payload["process"] == "ChunkAck") {
                            LOG_INFO("[MQTT] Chunk Ack recibido");
                        }
                    }


                    LOG_DEBUG("[MQTT] Lote confirmado. Chunks {}. Current index {}", chunk_count, initIndex);
                }
            }
            catch (const mqtt::exception& e) {
                LOG_ERROR("[MQTT] Error al publicar estado inicial: {}", e.what());
            }
        }

        

        std::string frameEndPayload = payloadSerializer_->generateFrameEndPayload();

        auto frameEndMsg = mqtt::make_message(TOPIC_CHANGES, frameEndPayload);
        frameEndMsg->set_qos(1);
        client_.publish(frameEndMsg)->wait();



        std::string frameSyncPayload = payloadSerializer_->generateFrameSyncPayload(snapshot.time());

        auto frameSyncMsg = mqtt::make_message(TOPIC_CHANGES, frameSyncPayload);
        frameSyncMsg->set_qos(1);
        client_.publish(frameSyncMsg)->wait();


        client_.unsubscribe(TOPIC_CONTROL_CHANGES)->wait();
    }

    std::string MqttOutput::getCurrentTimestampUTC() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm localTime = *std::localtime(&in_time_t);

        return std::string(fmt::format("{:%Y-%m-%dT%H:%M:%S}", localTime));
    }

} // namespace danasim
