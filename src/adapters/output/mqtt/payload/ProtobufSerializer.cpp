
#include "adapters/output/mqtt/payload/ProtobufSerializer.hpp"

#include "adapters/output/mqtt/payload/proto/messages.pb.h"

#include "app/logging/Logger.hpp"

namespace danasim {

    void ProtobufSerializer::serializeChunk(const Snapshot& snapshot, const int32_t chunkIndex, const int32_t totalChunks, const size_t chunkSize, std::string& result) const {
        // 1. Crear el mensaje Protobuf
        danasim::proto::SimulationFrame frame;

        frame.set_step_index(snapshot.step());
        frame.set_simulation_time(snapshot.time());

        frame.set_chunk_index(chunkIndex);
        frame.set_total_chunks(totalChunks);
        frame.set_is_last_chunk((chunkIndex+1) == totalChunks);

        // 2. Copiar los vectores de coordenadas (Zero-Copy logic si es posible, o iteradores)
        // MapGrid::changedX() devuelve std::vector<int32_t>
        const auto& vec_x = snapshot.changedX();
        const auto& vec_y = snapshot.changedY();

        size_t start = chunkIndex * chunkSize;
        size_t end = std::min(start + chunkSize, vec_x.size());

        // Usamos el método helper de Protobuf para añadir rangos (muy rápido)
        frame.mutable_changed_x()->Add(vec_x.begin() + start, vec_x.begin() + end);
        frame.mutable_changed_y()->Add(vec_y.begin() + start, vec_y.begin() + end);

        // 3. Serializar a string binario
        if (!frame.SerializeToString(&result)) {
            LOG_ERROR("[MQTT] Error serializando protobuf.");
        }
    }

} // namespace danasim
