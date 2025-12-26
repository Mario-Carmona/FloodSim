
#pragma once

#include "core/ports/SnapshotSerializerPort.hpp"

namespace danasim {

    class X3DOptimizedSnapshotSerializer final : public SnapshotSerializerPort {
    public:
        // Pasamos la configuración en el constructor para mantener serialize() limpio
        struct Config {
            float terrainColor[3] = { 0.4f, 0.5f, 0.1f };
            float waterColor[3] = { 0.0f, 0.4f, 0.8f };
            float waterTransparency = 0.5f;
            uint32_t tileSize = 1024;    // Tamaño deseado de cada pieza
            float lodDistance = 5000.0f; // Distancia a la que se oculta el detalle
        };

        explicit X3DOptimizedSnapshotSerializer(Config config = {});

        void serializeToStr(const Snapshot& snapshot, std::string& result) const override;

        // Sobrescribimos serializeToFile para gestionar la carpeta de tiles
        void serializeToFile(const Snapshot& snapshot, const std::filesystem::path& path) const override;

    protected:
        // Genera el HTML maestro
        void serializeToStream(std::ostream& os, const Snapshot& snapshot) const override;

    private:
        Config config_;

        // Procesa una tesela (Terreno o Agua) y la guarda comprimida
        void processTile(const std::filesystem::path& folder, const Snapshot& snapshot,
            uint32_t tx, uint32_t tz, uint32_t x0, uint32_t z0,
            uint32_t w, uint32_t h, bool isWater) const;

        // Auxiliar para comprimir datos con zlib
        std::vector<char> compressGzip(const std::vector<float>& data) const;
    };

} // namespace danasim
