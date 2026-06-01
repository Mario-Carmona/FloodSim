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
#include "app/exception/Exception.hpp"

namespace floodsim::app::io::readers::file {

bool FileStaticReader::IsStaticLayer(const std::filesystem::path& data_path,
                                     const std::string& data_filename,
                                     const formats::file::FileStaticFormat& format) {
    switch (format) {
    case formats::file::FileStaticFormat::kIdrisi:
        return IdrisiReader::IsStaticLayer(data_path, data_filename);
    default:
        throw floodsim::app::exception::FloodSimException(fmt::format(
            "Unsupported file static format '{}' encountered in IsStaticLayer.",
            magic_enum::enum_name(format)));
    }
}

FileStaticReader::FileStaticReader(const std::filesystem::path& data_path,
                                   const std::string& data_filename)
    : StaticReader(), data_path_(data_path), data_filename_(data_filename) {}

} // namespace floodsim::app::io::readers::file
