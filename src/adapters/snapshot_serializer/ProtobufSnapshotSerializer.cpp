
#include "adapters/snapshot_serializer/ProtobufSnapshotSerializer.hpp"

#include "proto/messages.pb.h"

#include "app/logging/Logger.hpp"

namespace danasim {

    void ProtobufSnapshotSerializer::serializeToStr(const Snapshot& snapshot, std::string& result) const {
        if (!snapshot.isValid()) {
            throw std::runtime_error("Intento de serializar un Snapshot inválido.");
        }

        const auto& grid = *snapshot.grid();

        // 1. Crear el mensaje Protobuf
        danasim::proto::SimulationFrame frame;
        frame.set_step_index(snapshot.step());
        frame.set_simulation_time(snapshot.time());

        // 2. Copiar los vectores de coordenadas (Zero-Copy logic si es posible, o iteradores)
        // MapGrid::changedX() devuelve std::vector<int32_t>
        const auto& vec_x = grid.changedX();
        const auto& vec_y = grid.changedY();

        // Usamos el método helper de Protobuf para añadir rangos (muy rápido)
        frame.mutable_changed_x()->Add(vec_x.begin(), vec_x.end());
        frame.mutable_changed_y()->Add(vec_y.begin(), vec_y.end());

        // 3. Serializar a string binario
        if (!frame.SerializeToString(&result)) {
            LOG_ERROR("[MQTT] Error serializando protobuf.");
        }
    }

    void ProtobufSnapshotSerializer::serializeToFile(const Snapshot& snapshot, const std::filesystem::path& path) const {

    }

    void ProtobufSnapshotSerializer::serializeToStream(std::ostream& os, const Snapshot& snapshot) const {
        
    }

} // namespace danasim
