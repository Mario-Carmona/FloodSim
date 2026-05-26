/**
 * @file IdrisiWriter.hpp
 * @brief Implementation of a static writer for IDRISI raster formats.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "app/core/grid/GridMetadata.hpp"
#include "app/io/writers/file/static/FileStaticWriter.hpp"

namespace floodsim {

/**
 * @brief Writer for IDRISI (.img / .doc) static grid files.
 */
class IdrisiWriter : public FileStaticWriter {
public:
    /**
     * @brief Constructs a new IdrisiWriter.
     * @param data_filename Base file name of the target resource (without extension).
     */
    explicit IdrisiWriter(const std::string& data_filename);

    virtual ~IdrisiWriter() override = default;

    /**
     * @brief Saves float data to an IDRISI image file along with its metadata document.
     * @param data_path Destination directory.
     * @param data Vector containing the float data.
     * @param metadata Spatial and structural properties of the grid.
     */
    void Save(const std::filesystem::path& data_path,
        const std::vector<float>& data,
        const GridMetadata& metadata) const override;

    /**
     * @brief Saves 8-bit integer data to an IDRISI image file along with its metadata document.
     * @param data_path Destination directory.
     * @param data Vector containing the int8_t data.
     * @param metadata Spatial and structural properties of the grid.
     */
    void Save(const std::filesystem::path& data_path,
        const std::vector<int8_t>& data,
        const GridMetadata& metadata) const override;

protected:
    /**
     * @brief Templated method for high-performance data writing to the IDRISI format.
     * @tparam T The data type to write.
     * @param data_path Destination directory.
     * @param data Vector containing the data.
     * @param metadata Spatial and structural properties of the grid.
     */
    template <typename T>
    void SaveData(const std::filesystem::path& data_path,
        const std::vector<T>& data,
        const GridMetadata& metadata) const;
};

} // namespace floodsim
