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

#include "app/core/ports/InputPort.hpp"
#include "app/adapters/input/readers/file/static/FileStaticFormat.hpp"
#include "app/adapters/input/readers/file/dynamic/FileDynamicFormat.hpp"

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
        explicit FileInput(const std::filesystem::path& dataPath, const std::string& datasetName, const FileStaticFormat& staticFormat, const FileDynamicFormat& dynamicFormat);

        virtual ~FileInput() = default;

        std::unique_ptr<Reader> generateReader(std::string name, bool isStatic) const override;

        bool isStaticLayer(const std::string& name) const override;

        inline const std::string& getDatasetName() const { return datasetName_; }

    private:
        std::filesystem::path inputPath_;

        std::string datasetName_;

        FileStaticFormat staticFormat_;
        FileDynamicFormat dynamicFormat_;
    };

} // namespace danasim
