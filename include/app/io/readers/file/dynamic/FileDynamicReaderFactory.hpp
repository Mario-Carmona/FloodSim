/**
 * @file FileDynamicReaderFactory.hpp
 * @brief Factory interface for creating dynamic reader instances.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "app/io/formats/file/FileDynamicFormat.hpp"
#include "app/io/readers/DynamicReader.hpp"

namespace floodsim::app::io::readers::file {

/**
 * @brief Factory utility to safely instantiate concrete DynamicReader subclasses.
 */
class FileDynamicReaderFactory {
public:
    /**
     * @brief Factory method to create a concrete instance of a DynamicReader.
     * @param format Requested dynamic file format.
     * @param data_path Directory where the file resides.
     * @param data_filename Name of the data file.
     * @return A std::unique_ptr pointing to the created DynamicReader.
     * @throws floodsim::app::exception::FloodSimException If the requested format is unsupported or fails to build.
     */
    static std::unique_ptr<DynamicReader> Create(
        const formats::file::FileDynamicFormat& format, const std::filesystem::path& data_path,
        const std::string& data_filename);
};

} // namespace floodsim::app::io::readers::file
