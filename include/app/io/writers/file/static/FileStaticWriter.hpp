/**
 * @file FileStaticWriter.hpp
 * @brief Interface for file-based static layer writers.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <string>

#include "app/io/writers/StaticWriter.hpp"

namespace floodsim::app::io::writers::file {

/**
 * @brief Base writer implementation for exporting static geographical data files.
 */
class FileStaticWriter : public StaticWriter {
public:
    /**
     * @brief Constructs a new FileStaticWriter.
     * @param data_filename File name of the target resource to be written.
     */
    explicit FileStaticWriter(const std::string& data_filename);

    virtual ~FileStaticWriter() override = default;

protected:
    std::string data_filename_;
};

} // namespace floodsim::app::io::writers::file
