
#pragma once

#include <string>
#include <filesystem>
#include <unordered_set>

#include "core/ports/InputPort.hpp"

namespace danasim {

    /**
     * @brief File-based input adapter (GIS / IDRISI).
     */
    class FileInput : public InputPort {
    public:
        explicit FileInput(const std::string& inputPath);

        void load(MapGrid& grid) override;

    private:
        // Carga automática desde GDAL para capas Principales
        void loadLayerFromGDAL(MapGrid& grid, const std::filesystem::path& path, LayerId layerId);
        
        // Lógica para capas Secundarias (derivadas)
        void initializeSecondaryLayer(MapGrid& grid, LayerId id);
        
        std::filesystem::path findRstFile(const std::filesystem::path& directory);
    
        std::string inputPath_;
    };

} // namespace danasim
