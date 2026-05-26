/**
 * @file FileDynamicReader.hpp
 * @brief Interface for file-based dynamic layer readers.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <filesystem>
#include <string>

#include "app/io/readers/DynamicReader.hpp"

namespace floodsim {

/**
 * @brief Base reader implementation for processing dynamic timeseries data files.
 */
class FileDynamicReader : public DynamicReader {
public:
    /**
     * @brief Constructs a new FileDynamicReader.
     * @param data_path Target directory containing the dynamic files.
     * @param data_filename Base file name of the dynamic resource.
     */
    FileDynamicReader(const std::filesystem::path& data_path,
                      const std::string& data_filename);

    virtual ~FileDynamicReader() override = default;

protected:
    std::filesystem::path data_path_;
    std::string data_filename_;
};

} // namespace floodsim
