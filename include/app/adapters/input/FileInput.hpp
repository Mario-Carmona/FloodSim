/**
 * @file FileInput.hpp
 * @brief Input adapter for loading GIS and simulation data from files.
 *
 * Implements the InputPort interface to handle the ingestion of static rasters
 * and dynamic time-series data from structured file system directories.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "app/core/ports/InputPort.hpp"
#include "app/io/formats/file/FileDynamicFormat.hpp"
#include "app/io/formats/file/FileStaticFormat.hpp"
#include "app/io/readers/Reader.hpp"

namespace floodsim {

/**
 * @class FileInput
 * @brief Concrete implementation of InputPort for file-system-based data sources.
 *
 * Dispatches layer loading to specialized static or dynamic file readers
 * based on format configurations.
 */
class FileInput : public InputPort {
public:
    /**
     * @brief Constructs the file input adapter.
     * @param data_path The root directory containing the dataset folders.
     * @param dataset_name The identification name of the target dataset folder.
     * @param static_format Target format type for static geographical layers.
     * @param dynamic_format Target format type for time-variant simulation layers.
     */
    FileInput(const std::filesystem::path& data_path,
              const std::string& dataset_name,
              const FileStaticFormat& static_format,
              const FileDynamicFormat& dynamic_format);

    virtual ~FileInput() override = default;

    /**
     * @brief Generates a specific Reader instance for the given layer.
     * @param name Name of the layer file/dataset to read.
     * @param is_static True if the target layer data is time-invariant.
     * @return A std::unique_ptr containing the concrete Reader instance.
     */
    std::unique_ptr<Reader> GenerateReader(const std::string& name,
                                           bool is_static) const override;

    /**
     * @brief Validates if a specific layer is registered as static.
     * @param name Name of the layer to check.
     * @return true if the layer format is static, false otherwise.
     */
    bool IsStaticLayer(const std::string& name) const override;

    /**
     * @brief Retrieves the active dataset identifier name.
     * @return Constant reference to the dataset name string.
     */
    const std::string& GetDatasetName() const override { return dataset_name_; }

private:
    std::filesystem::path input_path_;
    std::string dataset_name_;
    FileStaticFormat static_format_;
    FileDynamicFormat dynamic_format_;
};

} // namespace floodsim
