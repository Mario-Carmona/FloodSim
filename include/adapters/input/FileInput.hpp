/**
 * @file FileInput.hpp
 * @brief Input adapter for loading GIS and Simulation data from files.
 *
 * Handles the reading of static rasters (via GDAL) and dynamic data
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

#include "ports/InputPort.hpp"

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
        explicit FileInput(const std::filesystem::path& inputPath, const std::string& staticFormat, const std::string& dynamicFormat);

        virtual ~FileInput() = default;

        std::unique_ptr<Reader> generateReader(std::string name, bool isStatic) override;

    private:
        std::filesystem::path inputPath_;

        std::string staticFormat_;
        std::string dynamicFormat_;
    };

} // namespace danasim
