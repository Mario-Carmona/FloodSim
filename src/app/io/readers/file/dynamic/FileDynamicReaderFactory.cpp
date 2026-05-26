/**
 * @file FileDynamicReaderFactory.cpp
 * @brief Factory implementation for the generation of dynamic file readers.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "app/io/readers/file/dynamic/FileDynamicReaderFactory.hpp"

#include <memory>
#include <stdexcept>

#include <fmt/core.h>
#include <magic_enum/magic_enum.hpp>

#include "app/io/readers/file/dynamic/FileDynamicReader.hpp"
#include "app/io/readers/file/dynamic/HifReader.hpp"

namespace floodsim::app::io::readers::file {

std::unique_ptr<DynamicReader> FileDynamicReaderFactory::Create(
        const formats::file::FileDynamicFormat& format, const std::filesystem::path& data_path,
        const std::string& data_filename) {
    switch (format) {
    case formats::file::FileDynamicFormat::kHif:
        return std::make_unique<HifReader>(data_path, data_filename);
    default:
        throw std::runtime_error(fmt::format(
            "Failed to create DynamicReader: Unsupported format '{}'.",
            magic_enum::enum_name(format)));
    }
}

} // namespace floodsim::app::io::readers::file
