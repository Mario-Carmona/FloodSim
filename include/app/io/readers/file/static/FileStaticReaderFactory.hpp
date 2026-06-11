/**
 * @file FileStaticReaderFactory.hpp
 * @brief Factory interface for creating static reader instances.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "app/io/formats/file/FileStaticFormat.hpp"
#include "app/io/readers/StaticReader.hpp"

namespace floodsim::app::io::readers::file {

/**
 * @brief Factory pattern utility to safely instantiate distinct StaticReader types.
 */
class FileStaticReaderFactory {
public:
    /**
     * @brief Factory method to create a concrete instance of a StaticReader.
     * @param format Requested static file format.
     * @param data_path Directory where the file resides.
     * @param data_filename Name of the data file.
     * @return A std::unique_ptr pointing to the created StaticReader.
     * @throws floodsim::app::exception::FloodSimException If requested format cannot be built or is unsupported.
     */
    static std::unique_ptr<StaticReader> Create(
        const formats::file::FileStaticFormat& format, const std::filesystem::path& data_path,
        const std::string& data_filename);
};

} // namespace floodsim::app::io::readers::file
