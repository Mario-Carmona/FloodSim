
#pragma once

#include "core/ports/SnapshotSerializerPort.hpp"

namespace danasim {

    class X3DSnapshotSerializer final : public SnapshotSerializerPort {
    public:
        // Pasamos la configuración en el constructor para mantener serialize() limpio
        struct ColorConfig {
            float terrainColor[3] = { 0.6f, 0.4f, 0.2f };
            float waterColor[3] = { 0.0f, 0.5f, 1.0f };
            float transparency = 0.5f;
        };

        explicit X3DSnapshotSerializer(ColorConfig colorConfig = {});

        void serializeToStr(const Snapshot& snapshot, std::string& result) const override;
        void serializeToFile(const Snapshot& snapshot, const std::filesystem::path& path) const override;

    protected:
        void serializeToStream(std::ostream& os, const Snapshot& snapshot) const override;

    private:
        ColorConfig colorConfig_;

        // Método interno de alto rendimiento compartido
        void writeHeights(std::ostream& os, const float* data, uint32_t count) const;
    };

} // namespace danasim
