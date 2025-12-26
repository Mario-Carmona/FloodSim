
#include "adapters/input/FileInput.hpp"

#include <stdexcept>
#include <gdal_priv.h>

#include "app/logging/Logger.hpp"
#include "core/grid/LayerTypes.hpp"

namespace danasim {

    FileInput::FileInput(const std::string& inputPath)
        : inputPath_(inputPath) 
    {
        GDALAllRegister(); // Inicializar drivers de GDAL
    }

    void FileInput::load(MapGrid& grid) {
        std::filesystem::path root(inputPath_);

        // El MapGrid se crea con la primera capa Principal que encuentre para definir dimensiones
        // Obtenemos los descriptores del Grid (que ya vienen definidos en el MapGrid)

        for (auto& value : magic_enum::enum_values<LayerId>()) {
			const auto& desc = MapGrid::getDescriptor(value);

            if (desc.role == LayerRole::Principal) {
                // Buscamos la carpeta con el nombre de la capa (ej: "elevation")
                loadLayerFromGDAL(grid, root / desc.name, value);
            }
        }

        for (auto& value : magic_enum::enum_values<LayerId>()) {
            const auto& desc = MapGrid::getDescriptor(value);

            if (desc.role == LayerRole::Secondary) {
                initializeSecondaryLayer(grid, value);
            }
        }

        LOG_INFO("Input port fully loaded from GDAL datasets.");
    }

    void FileInput::loadLayerFromGDAL(MapGrid& grid, const std::filesystem::path& directory, LayerId layerId) {
        if (layerId == LayerId::Roughness) {
            float* roughnessData = grid.layerData(LayerId::Roughness);

            for (uint32_t i = 0; i < grid.cellCount(); ++i) {
                roughnessData[i] = 0.0f;
            }
        }
        else {
            std::filesystem::path rstPath = findRstFile(directory);

            GDALDataset* dataset = (GDALDataset*)GDALOpen(rstPath.string().c_str(), GA_ReadOnly);
            if (!dataset) {
                throw std::runtime_error("GDAL: Could not open " + rstPath.string());
            }

            uint32_t dsW = dataset->GetRasterXSize();
            uint32_t dsH = dataset->GetRasterYSize();

            // Si es la primera capa, inicializamos el MapGrid
            if (grid.cellCount() == 0) {
                float cellSize = 1.0;

                double geoTransform[6];
                if (dataset->GetGeoTransform(geoTransform) == CE_None) {
                    cellSize = static_cast<float>(geoTransform[1]); // Resolución X
                }

                grid.init(dsW, dsH, cellSize);
            }
            // Validar que todas las capas tengan las mismas dimensiones
            else if (dsW != grid.width() || dsH != grid.height()) {
                GDALClose(dataset);
                throw std::runtime_error("GDAL: Dimension mismatch in " + rstPath.string());
            }

            GDALRasterBand* band = dataset->GetRasterBand(1);
            float* targetPtr = grid.layerData(layerId);

            // Lectura directa de GDAL al buffer del MapGrid
            CPLErr err = band->RasterIO(
                GF_Read,
                0, 0,
                static_cast<int>(grid.width()),
                static_cast<int>(grid.height()),
                targetPtr,
                static_cast<int>(grid.width()),
                static_cast<int>(grid.height()),
                GDT_Float32,
                0, 0
            );

            GDALClose(dataset);

            if (err != CE_None) {
                throw std::runtime_error("GDAL: Error reading pixels from " + rstPath.string());
            }

            LOG_DEBUG("Loaded layer {} from {}", magic_enum::enum_name(layerId), rstPath.filename().string());
        }
    }

    void FileInput::initializeSecondaryLayer(MapGrid& grid, LayerId id) {
        switch (id) {
        case LayerId::CellState: {
            LOG_DEBUG("Computing CellState from WaterDepth...");

            const float* waterDepthData = grid.layerData(LayerId::WaterDepth);
            float* cellStateData = grid.layerData(LayerId::CellState);

            for (uint32_t i = 0; i < grid.cellCount(); ++i) {
                cellStateData[i] = (waterDepthData[i] > CellStateValues::WaterThreshold) ? CellStateValues::Dynamic : CellStateValues::Empty;
            }
            break;
        }
        default:
            // No insertamos en initializedLayers_ si llega aquí
            LOG_ERROR("Advertencia: No hay lógica de inicialización para la capa secundaria: {}", magic_enum::enum_name(id));
            break;
        }
    }

    std::filesystem::path FileInput::findRstFile(const std::filesystem::path& directory) {
        if (!std::filesystem::exists(directory)) {
            throw std::runtime_error("Input directory missing: " + directory.string());
        }
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.path().extension() == ".rst") return entry.path();
        }
        throw std::runtime_error("No .rst file found in: " + directory.string());
    }

} // namespace danasim
