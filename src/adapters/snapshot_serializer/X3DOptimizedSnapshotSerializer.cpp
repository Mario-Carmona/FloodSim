
#include "adapters/snapshot_serializer/X3DOptimizedSnapshotSerializer.hpp"

#include <fstream>
#include <format>
#include <future>
#include <algorithm>
#include <zlib.h>

namespace danasim {

    X3DOptimizedSnapshotSerializer::X3DOptimizedSnapshotSerializer(Config config)
        : config_(config) {}

    void X3DOptimizedSnapshotSerializer::serializeToStr(const Snapshot& snapshot, std::string& result) const {
        if (!snapshot.isValid()) {
            throw std::runtime_error("Intento de serializar un Snapshot inválido.");
        }

        // 2. RESERVA Y OPTIMIZACIÓN
        result.reserve(snapshot.grid()->cellCount() * 10 + 2048);

        // 3. FLUJO CENTRALIZADO (C++20 move-to-stream)
        std::ostringstream oss(std::move(result));
        serializeToStream(oss, snapshot);
    }

    void X3DOptimizedSnapshotSerializer::serializeToFile(const Snapshot& snapshot, const std::filesystem::path& path) const {
        // 1. Crear carpeta para los binarios (misma ruta que el html)
        std::filesystem::path folder = path.parent_path() / "data";
        std::filesystem::create_directories(folder);

        // 2. Generar el archivo maestro HTML
        std::ofstream htmlFile(path);
        serializeToStream(htmlFile, snapshot);

        // 3. Lanzar procesamiento de TILES en PARALELO
        const auto& grid = *snapshot.grid();
        uint32_t nX = (grid.width() + config_.tileSize - 1) / config_.tileSize;
        uint32_t nZ = (grid.height() + config_.tileSize - 1) / config_.tileSize;

        std::vector<std::future<void>> tasks;

        for (uint32_t tz = 0; tz < nZ; ++tz) {
            for (uint32_t tx = 0; tx < nX; ++tx) {
                uint32_t x0 = tx * config_.tileSize;
                uint32_t z0 = tz * config_.tileSize;
                uint32_t currW = std::min(config_.tileSize, grid.width() - x0);
                uint32_t currH = std::min(config_.tileSize, grid.height() - z0);

                // Tarea para Terreno
                tasks.push_back(std::async(std::launch::async, [=, &snapshot] {
                    processTile(folder, snapshot, tx, tz, x0, z0, currW, currH, false);
                    }));

                // Tarea para Agua (Opcional, podrías unirlas para más velocidad)
                tasks.push_back(std::async(std::launch::async, [=, &snapshot] {
                    processTile(folder, snapshot, tx, tz, x0, z0, currW, currH, true);
                    }));
            }
        }
        // Esperar a que terminen todos los hilos
        for (auto& t : tasks) t.get();
    }

    void X3DOptimizedSnapshotSerializer::serializeToStream(std::ostream& os, const Snapshot& snapshot) const {
        const auto& grid = *snapshot.grid();

        os << std::format(R"(<!DOCTYPE html><html><head>
            <script src="https://www.x3dom.org/download/x3dom.js"></script>
            <link rel="stylesheet" href="https://www.x3dom.org/download/x3dom.css">
            </head><body style='margin:0; overflow:hidden;'><x3d width='100%' height='100%'>
            <scene><background skyColor='0.5 0.7 1'></background>)");

        uint32_t nX = (grid.width() + config_.tileSize - 1) / config_.tileSize;
        uint32_t nZ = (grid.height() + config_.tileSize - 1) / config_.tileSize;

        for (uint32_t tz = 0; tz < nZ; ++tz) {
            for (uint32_t tx = 0; tx < nX; ++tx) {
                uint32_t x0 = tx * config_.tileSize;
                uint32_t z0 = tz * config_.tileSize;
                uint32_t w = std::min(config_.tileSize, grid.width() - x0);
                uint32_t h = std::min(config_.tileSize, grid.height() - z0);

                // LOD para Terreno
                os << std::format(R"(
                <LOD range='{}'>
                    <transform translation='{} 0 {}'>
                        <shape>
                            <appearance><material diffuseColor='{} {} {}'></material></appearance>
                            <binaryGeometry vertexCount='{}' primType='"POINTS"' position='data/t_{}_{}.bin.gz'></binaryGeometry>
                        </shape>
                        <shape>
                            <appearance><material diffuseColor='{} {} {}' transparency='{}'></material></appearance>
                            <binaryGeometry vertexCount='{}' primType='"POINTS"' position='data/w_{}_{}.bin.gz'></binaryGeometry>
                        </shape>
                    </transform>
                    <worldInfo info='"far"'></worldInfo>
                </LOD>)",
                    config_.lodDistance, x0, z0,
                    config_.terrainColor[0], config_.terrainColor[1], config_.terrainColor[2], (w * h), tx, tz,
                    config_.waterColor[0], config_.waterColor[1], config_.waterColor[2], config_.waterTransparency, (w * h), tx, tz);
            }
        }
        os << "</scene></x3d></body></html>";
    }

    void X3DOptimizedSnapshotSerializer::processTile(const std::filesystem::path& folder, const Snapshot& snapshot,
        uint32_t tx, uint32_t tz, uint32_t x0, uint32_t z0,
        uint32_t w, uint32_t h, bool isWater) const {
        const auto& grid = *snapshot.grid();
        const float* terrain = grid.layerData(LayerId::Elevation);
        const float* water = grid.layerData(LayerId::WaterDepth);

        std::vector<float> buffer;
        buffer.reserve(w * h * 3);

        bool hasContent = false;
        for (uint32_t lz = 0; lz < h; ++lz) {
            for (uint32_t lx = 0; lx < w; ++lx) {
                uint32_t idx = (z0 + lz) * grid.width() + (x0 + lx);

                float y = isWater ? (terrain[idx] + water[idx]) : terrain[idx];
                if (isWater && water[idx] > 0.001f) hasContent = true;

                buffer.push_back((float)lx); buffer.push_back(y); buffer.push_back((float)lz);
            }
        }

        if (isWater && !hasContent) return; // No crear archivo de agua si está seco

        auto compressed = compressGzip(buffer);
        std::string prefix = isWater ? "w_" : "t_";
        std::ofstream out(folder / std::format("{}{}_{}.bin.gz", prefix, tx, tz), std::ios::binary);
        out.write(compressed.data(), compressed.size());
    }

    std::vector<char> X3DOptimizedSnapshotSerializer::compressGzip(const std::vector<float>& data) const {
        z_stream zs;
        memset(&zs, 0, sizeof(zs));
        deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);

        zs.next_in = (Bytef*)data.data();
        zs.avail_in = data.size() * sizeof(float);

        std::vector<char> out;
        char temp[32768];
        do {
            zs.next_out = (Bytef*)temp;
            zs.avail_out = sizeof(temp);
            deflate(&zs, Z_FINISH);
            out.insert(out.end(), temp, temp + (sizeof(temp) - zs.avail_out));
        } while (zs.avail_out == 0);

        deflateEnd(&zs);
        return out;
    }

} // namespace danasim
