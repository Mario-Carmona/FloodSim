/**
 * @file HifReader.hpp
 * @brief Implementation of a dynamic reader for HIF timeseries formats.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

#include "app/core/grid/GridMetadata.hpp"
#include "app/io/readers/file/dynamic/FileDynamicReader.hpp"

namespace floodsim {

/**
 * @brief Dynamic reader for HIF (Hierarchical Image Format) timeseries data.
 *
 * Processes JSON metadata to handle chronological sequences of static raster
 * frames (e.g., IDRISI files) updating the active frame over simulation time.
 */
class HifReader : public FileDynamicReader {
public:
    /**
     * @brief Constructs a new HifReader and parses its associated JSON metadata.
     * @param data_path Target directory containing the dataset.
     * @param data_filename Base file name of the HIF resource (excluding _metadata.json).
     */
    HifReader(const std::filesystem::path& data_path,
              const std::string& data_filename);

    virtual ~HifReader() override = default;

    /**
     * @brief Reads the current frame's float data.
     * @param data Vector to be populated.
     */
    void Read(std::vector<float>& data) const override;

    /**
     * @brief Reads the current frame's 8-bit integer data.
     * @param data Vector to be populated.
     */
    void Read(std::vector<int8_t>& data) const override;

    /**
     * @brief Retrieves spatial and structural metadata for the dynamic layer.
     * @return A GridMetadata object.
     */
    GridMetadata ReadMetadata() const override;

    /**
     * @brief Updates the internal frame pointer based on the simulation time.
     * @param current_time The current simulation timestamp.
     * @return True if a new frame was loaded to replace the current one, false otherwise.
     */
    bool Update(std::chrono::system_clock::time_point current_time) override;

    /**
     * @brief Returns the spatial downgrade factor for this dynamic layer.
     * @return The integer downgrade factor.
     */
    unsigned int GetDowngradeFactor() const override;

protected:
    unsigned int steps_;
    unsigned int downgrade_factor_;
    std::vector<std::chrono::system_clock::time_point> timestamps_;
    std::vector<std::string> filenames_;
    unsigned int current_frame_;

    /**
     * @brief Templated method to safely dispatch read requests for the active frame.
     * @tparam T The data type to read.
     * @param data Vector to be populated.
     */
    template <typename T>
    void ReadData(std::vector<T>& data) const;
};

} // namespace floodsim
