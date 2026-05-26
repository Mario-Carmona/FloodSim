/**
 * @file FileStaticReader.cpp
 * @brief Implementation details for the FileStaticReader class.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "app/io/readers/file/static/FileStaticReader.hpp"

#include <stdexcept>

#include <fmt/core.h>
#include <magic_enum/magic_enum.hpp>

#include "app/io/readers/file/static/IdrisiReader.hpp"

namespace floodsim {

bool FileStaticReader::IsStaticLayer(const std::filesystem::path& data_path,
                                     const std::string& data_filename,
                                     const FileStaticFormat& format) {
    switch (format) {
    case FileStaticFormat::kIdrisi:
        return IdrisiReader::IsStaticLayer(data_path, data_filename);
    default:
        throw std::runtime_error(fmt::format(
            "Unsupported file static format '{}' encountered in IsStaticLayer.",
            magic_enum::enum_name(format)));
    }
}

FileStaticReader::FileStaticReader(const std::filesystem::path& data_path,
                                   const std::string& data_filename)
    : StaticReader(), data_path_(data_path), data_filename_(data_filename) {}

} // namespace floodsim
