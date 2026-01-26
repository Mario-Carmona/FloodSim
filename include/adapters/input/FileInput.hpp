/**
 * @file FileInput.hpp
 * @brief Input adapter for loading GIS and Simulation data from files.
 *
 * Handles the reading of static rasters (via GDAL) and dynamic streamed data
 * (via HDF5), populating the MapGrid structures accordingly.
 *
 * @version 1.0
 * @date 2026
 * @copyright Copyright (c) 2026 Danasim
 */

#pragma once

#include <string>
#include <filesystem>
#include <vector>

#include "core/ports/InputPort.hpp"

namespace danasim {

    /**
     * @class FileInput
     * @brief Concrete implementation of InputPort for file-based data sources.
     *
     * This class uses the GDAL library to read standard GIS formats (GeoTiff, RST)
     * and custom logic for HDF5 time-series data.
     */
    class FileInput : public InputPort {
    public:
        /**
         * @brief Constructs the file input adapter.
         * @param inputPath The root directory containing the scenario data.
         * @throw std::runtime_error If GDAL drivers cannot be registered.
         */
        explicit FileInput(const std::filesystem::path& inputPath);

        /**
         * @brief Loads all configured layers into the simulation grid.
         *
         * Iterates through the LayerId enumeration, determining whether to load
         * data from disk (Principal layers) or initialize derived data (Secondary layers).
         *
         * @param grid The main simulation state grid to populate.
         * @param streamedLayerManager Manager for handling time-series (HDF5) data.
         * @param timeStep The simulation time step (used for HDF5 caching).
         */
        void load(MapGrid& grid, StreamedLayerManager& streamedLayerManager, float timeStep) override;

    private:
        /**
         * @brief Loads a static raster layer using GDAL.
         * * Performs a bulk read of the raster band for maximum performance.
         *
         * @param grid The target map grid.
         * @param directory The directory containing the raster file.
         * @param layerId The specific layer ID being loaded.
         */
        void loadLayerFromGDAL(MapGrid& grid, const std::filesystem::path& directory, LayerId layerId);

        /**
         * @brief Configures the HDF5 streamer for a specific layer.
         */
        void loadLayerFromHDF5(MapGrid& grid, StreamedLayerManager& streamedLayerManager, const std::filesystem::path& directory, float timeStep, LayerId layerId);
        
        /**
         * @brief Initializes derived layers (e.g., empty buffers, calculated constants).
         */
        void initializeSecondaryLayer(MapGrid& grid, LayerId id);
        
        // --- Helpers ---

        /**
         * @brief Scans a directory for a specific file extension.
         * @return The full path to the first matching file found.
         * @throw std::runtime_error If the file is missing or the directory is invalid.
         */
        [[nodiscard]]
        std::filesystem::path findFileWithExtension(const std::filesystem::path& directory, std::string_view extension);
    
        std::filesystem::path inputPath_;
    };

} // namespace danasim
