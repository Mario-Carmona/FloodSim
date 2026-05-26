/**
 * @file FileStaticWriterFactory.hpp
 * @brief Factory interface for creating static writer instances.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <memory>
#include <string>

#include "app/io/formats/file/FileStaticFormat.hpp"
#include "app/io/writers/StaticWriter.hpp"

namespace floodsim {

/**
 * @brief Factory utility to safely instantiate concrete StaticWriter subclasses.
 */
class FileStaticWriterFactory {
public:
    /**
     * @brief Factory method to create a concrete instance of a StaticWriter.
     * @param format Requested static file format.
     * @param data_filename Name of the output data file.
     * @return A std::unique_ptr pointing to the created StaticWriter.
     * @throws std::runtime_error If the requested format is unsupported.
     */
    static std::unique_ptr<StaticWriter> Create(
        const FileStaticFormat& format, const std::string& data_filename);
};

} // namespace floodsim
