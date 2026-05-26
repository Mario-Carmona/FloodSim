/**
 * @file FileDynamicReader.cpp
 * @brief Implementation details for the FileDynamicReader class.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "app/io/readers/file/dynamic/FileDynamicReader.hpp"

namespace floodsim::app::io::readers::file {

FileDynamicReader::FileDynamicReader(const std::filesystem::path& data_path,
                                     const std::string& data_filename)
    : DynamicReader(), data_path_(data_path), data_filename_(data_filename) {}

} // namespace floodsim::app::io::readers::file
