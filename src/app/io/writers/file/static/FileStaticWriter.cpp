/**
 * @file FileStaticWriter.cpp
 * @brief Implementation details for the FileStaticWriter class.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "app/io/writers/file/static/FileStaticWriter.hpp"

namespace floodsim::app::io::writers::file {

FileStaticWriter::FileStaticWriter(const std::string& data_filename)
    : StaticWriter(), data_filename_(data_filename) {}

} // namespace floodsim::app::io::writers::file
