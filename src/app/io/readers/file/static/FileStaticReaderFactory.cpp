/**
 * @file FileStaticReaderFactory.cpp
 * @brief Factory implementation for the generation of static file readers.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "app/io/readers/file/static/FileStaticReaderFactory.hpp"

#include <memory>
#include <stdexcept>

#include <fmt/core.h>
#include <magic_enum/magic_enum.hpp>

#include "app/io/readers/file/static/FileStaticReader.hpp"
#include "app/io/readers/file/static/IdrisiReader.hpp"

namespace floodsim::app::io::readers::file {

std::unique_ptr<StaticReader> FileStaticReaderFactory::Create(
        const formats::file::FileStaticFormat& format, const std::filesystem::path& data_path,
        const std::string& data_filename) {
    switch (format) {
    case formats::file::FileStaticFormat::kIdrisi:
        return std::make_unique<IdrisiReader>(data_path, data_filename);
    default:
        throw std::runtime_error(fmt::format(
            "Failed to create StaticReader: Unsupported format '{}'.",
            magic_enum::enum_name(format)));
    }
}

} // namespace floodsim::app::io::readers::file
