/**
 * @file IdrisiReader.hpp
 * @brief Implementation of a static reader for IDRISI raster formats.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "app/core/grid/GridMetadata.hpp"
#include "app/io/readers/file/static/FileStaticReader.hpp"

namespace floodsim {

/**
 * @brief Reader for IDRISI (.img / .doc) static grid files.
 */
class IdrisiReader : public FileStaticReader {
public:
    /**
     * @brief Checks if the given path and filename correspond to a valid IDRISI layer.
     * @param data_path Directory containing the IDRISI files.
     * @param data_filename Base name of the file (without extension).
     * @return True if the IDRISI image file exists, false otherwise.
     */
    static bool IsStaticLayer(const std::filesystem::path& data_path,
                              const std::string& data_filename);

    /**
     * @brief Constructs a new IdrisiReader.
     * @param data_path Target directory containing the data.
     * @param data_filename File name of the target resource.
     */
    IdrisiReader(const std::filesystem::path& data_path,
                 const std::string& data_filename);

    virtual ~IdrisiReader() override = default;

    /**
     * @brief Reads float data from the IDRISI image file.
     * @param data Vector to be populated with float data.
     */
    void Read(std::vector<float>& data) const override;

    /**
     * @brief Reads 8-bit integer data from the IDRISI image file.
     * @param data Vector to be populated with int8_t data.
     */
    void Read(std::vector<int8_t>& data) const override;

    /**
     * @brief Parses the IDRISI documentation (.doc) file to extract grid metadata.
     * @return A GridMetadata object describing spatial and structural properties.
     */
    GridMetadata ReadMetadata() const override;

protected:
    /**
     * @brief Templated method for high-performance data reading from the IDRISI image.
     * @tparam T The data type to read.
     * @param data Vector to be populated.
     */
    template <typename T>
    void ReadData(std::vector<T>& data) const;
};

} // namespace floodsim
