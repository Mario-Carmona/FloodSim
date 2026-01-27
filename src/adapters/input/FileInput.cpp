/**
 * @file FileInput.cpp
 * @brief Optimized implementation of file loading logic using GDAL and C++20.
 */

#include "adapters/input/FileInput.hpp"

#include <stdexcept>
#include <fmt/core.h>
#include <algorithm>
#include <memory>

 // GDAL Includes
#include <gdal_priv.h>
#include <cpl_conv.h>

// Magic Enum Includes
#include <magic_enum/magic_enum.hpp>

#include "app/logging/Logger.hpp"
#include "core/grid/LayerTypes.hpp"
#include "core/common/SimulationConstants.hpp"
#include "core/grid/MapGrid.hpp"

namespace danasim {

    // -------------------------------------------------------------------------
    // Custom Deleter for RAII GDAL Management
    // -------------------------------------------------------------------------
    struct GDALDatasetDeleter {
        void operator()(GDALDataset* ds) const {
            if (ds) {
                GDALClose(ds);
            }
        }
    };
    using GDALDatasetUniquePtr = std::unique_ptr<GDALDataset, GDALDatasetDeleter>;

    // -------------------------------------------------------------------------
    // Implementation
    // -------------------------------------------------------------------------

    FileInput::FileInput(const std::filesystem::path& inputPath)
        : inputPath_(inputPath) 
    {
        // Ideally, this should be done once in the Application setup, 
        // but it's safe to call multiple times.
        GDALAllRegister();
        LOG_INFO("FileInput initialized with root: {}", inputPath.string());
    }

    void FileInput::load(MapGrid& grid, StreamedLayerManager& streamedLayerManager, float timeStep) {
        LOG_INFO("Loading simulation layers...");

        std::filesystem::path root(inputPath_);

        // Iterate over all possible layers defined in the enum
        for (auto layerId : magic_enum::enum_values<LayerId>()) {
            const auto& desc = MapGrid::getDescriptor(layerId);

            try {
                if (desc.role == LayerRole::Principal) {
                    // Calculate path: root / "layer_name" (e.g., ./data/elevation)
                    auto layerDir = root / desc.name;

                    if (desc.source == LayerSource::InMemory) {
                        LOG_DEBUG("Loading static layer '{}' from GDAL", desc.name);
                        loadLayerFromGDAL(grid, layerDir, layerId);
                    }
                    else {
                        LOG_DEBUG("Configuring streamed layer '{}' from HDF5", desc.name);
                        loadLayerFromHDF5(grid, streamedLayerManager, layerDir, timeStep, layerId);
                    }
                }
                else {
                    // Secondary layers (derived data, scratch buffers)
                    initializeSecondaryLayer(grid, layerId);
                }
            }
            catch (const std::exception& ex) {
                // Enhance exception with context before re-throwing
                auto msg = fmt::format("Failed to load layer '{}': {}", desc.name, ex.what());
                LOG_ERROR("{}", msg);
                throw std::runtime_error(msg);
            }
        }

        // Final validation: Ensuring that the MapGrid has been initialized
        if (grid.rows() == 0 || grid.cols() == 0) {
            std::string msg = "MapGrid was not initialized. Check input files.";
            LOG_ERROR("{}", msg);
            throw std::runtime_error(msg);
        }

        LOG_INFO("All layers loaded successfully. Grid Size: {}x{}", grid.rows(), grid.cols());
    }

    void FileInput::loadLayerFromGDAL(MapGrid& grid, const std::filesystem::path& directory, LayerId layerId) {
        // 1. Locate the file (supports .rst or .tif logic via helper)
        std::filesystem::path filePath = findFileWithExtension(directory, ".rst");

        // 2. Open Dataset (RAII)
        GDALDatasetUniquePtr dataset(
            (GDALDataset*)GDALOpen(filePath.string().c_str(), GA_ReadOnly)
        );

        if (!dataset) {
            auto msg = fmt::format("GDAL failed to open file: {}", filePath.string());
            LOG_ERROR("{}", msg);
            throw std::runtime_error(msg);
        }

        int width = dataset->GetRasterXSize();
        int height = dataset->GetRasterYSize();

        if (layerId == LayerId::Elevation) {
            // 3. Initialize Dimensions
            double geoTransform[6];
            if (dataset->GetGeoTransform(geoTransform) == CE_None) {
                float cellSize = static_cast<float>(geoTransform[1]);

                float mapOriginX = static_cast<float>(geoTransform[0]);
                float mapOriginY = static_cast<float>(geoTransform[3]);

                grid.init(height, width, cellSize, mapOriginX, mapOriginY);
            }
            else {
                std::string msg = "Not found IDRISI metada.";
                LOG_ERROR("{}", msg);
                throw std::runtime_error(msg);
            }
        }
        else {
            // 3. Validate Dimensions
            if (width != grid.cols() || height != grid.rows()) {
                auto msg = fmt::format(
                    "Dimension mismatch. Grid is {}x{}, but file '{}' is {}x{}", 
                    grid.cols(), grid.rows(), filePath.filename().string(), width, height
                );
                LOG_ERROR("{}", msg);
                throw std::runtime_error(msg);
            }
        }

        // 4. Fetch Raster Band
        GDALRasterBand* band = dataset->GetRasterBand(1); // Bands are 1-indexed
        if (!band) {
            std::string msg = "File has no raster bands";
            LOG_ERROR("{}", msg);
            throw std::runtime_error(msg);
        }

        // 5. Bulk Read
        // Reading the whole block at once is significantly faster than GetValue() per pixel.
        CPLErr err;

        if (MapGrid::getDescriptor(layerId).dataType == LayerDataType::Float32) {
            float* targetPtr = grid.getLayer<float>(layerId).data();

            err = band->RasterIO(GF_Read, 0, 0, width, height,
                targetPtr, width, height,
                GDT_Float32, 0, 0);
        }
        else {
            int8_t* targetPtr = grid.getLayer<int8_t>(layerId).data();

            err = band->RasterIO(GF_Read, 0, 0, width, height,
                targetPtr, width, height,
                GDT_Int8, 0, 0);
        }

        if (err != CE_None) {
            std::string msg = "GDAL RasterIO error while reading band";
            LOG_ERROR("{}", msg);
            throw std::runtime_error(msg);
        }
    }

    void FileInput::loadLayerFromHDF5(MapGrid& grid, StreamedLayerManager& streamedLayerManager, const std::filesystem::path& directory, float timeStep, LayerId layerId) {
        std::filesystem::path filePath = findFileWithExtension(directory, ".h5");

        // Delegate to the manager logic
        streamedLayerManager.initLayer(grid, filePath, timeStep, layerId);

        LOG_INFO("Registered HDF5 source: {}", filePath.string());
    }

    void FileInput::initializeSecondaryLayer(MapGrid& grid, LayerId id) {
        // Logic for derived layers.
        switch (id) {
        case LayerId::Roughness: {
            LOG_INFO("Setting Roughness...");

            float* roughnessData = grid.getLayer<float>(LayerId::Roughness).data();

            std::fill(roughnessData, roughnessData + grid.cellCount(), 0.0f);
            break;
        }
        case LayerId::WaterDepth: {
            LOG_INFO("Setting WaterDepth...");

            float* waterDepthData = grid.getLayer<float>(LayerId::WaterDepth).data();

            std::fill(waterDepthData, waterDepthData + grid.cellCount(), 0.0f);
            break;
        }
        default:
            auto msg = fmt::format(
                "There is no initialization logic for the secondary layer: {}",
                magic_enum::enum_name(id)
            );
            LOG_ERROR("{}", msg);
            throw std::runtime_error(msg);
            break;
        }
    }

    std::filesystem::path FileInput::findFileWithExtension(const std::filesystem::path& directory, std::string_view extension) {
        if (!std::filesystem::exists(directory)) {
            auto msg = fmt::format("Input directory missing: '{}'", directory.string());
            LOG_ERROR("{}", msg);
            throw std::runtime_error(msg);
        }

        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().extension() == extension) {
                return entry.path();
            }
        }

        auto msg = fmt::format("No file with extension '{}' found in '{}'", extension, directory.string());
        LOG_ERROR("{}", msg);
        throw std::runtime_error(msg);
    }

} // namespace danasim
