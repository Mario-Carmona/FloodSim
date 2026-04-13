
#include "adapters/output/mqtt/payload/JsonSerializer.hpp"

#include <fmt/core.h>
#include <fmt/chrono.h>

#include "logging/Logger.hpp"

#include <nlohmann/json.hpp>

namespace danasim {

    void JsonSerializer::serializeChunk(const Snapshot& snapshot, const ChangeList& changes, const int32_t chunkIndex, const int32_t totalChunks, const size_t chunkSize, std::string& result) const {
        // 1. Crear el mensaje JSON
        nlohmann::json frame;

        frame["simulation_time"] = fmt::format("{:%FT%T}", snapshot.time());

        frame["chunk_index"] = chunkIndex;
        frame["total_chunks"] = totalChunks;
        frame["is_last_chunk"] = (chunkIndex + 1) == totalChunks;

        // 2. Copiar los vectores de coordenadas (Zero-Copy logic si es posible, o iteradores)
        // MapGrid::changedX() devuelve std::vector<int32_t>
        const auto& vec_x = changes.x;
        const auto& vec_y = changes.y;

        size_t start = chunkIndex * chunkSize;
        size_t end = std::min(start + chunkSize, vec_x.size());

        // Usamos el método helper de Protobuf para añadir rangos (muy rápido)
        frame["changed_x"] = std::vector<GridIndexType>(vec_x.begin() + start, vec_x.begin() + end);
		frame["changed_y"] = std::vector<GridIndexType>(vec_y.begin() + start, vec_y.begin() + end);

        // 3. Serializar a string
        result = frame.dump();
    }

} // namespace danasim
