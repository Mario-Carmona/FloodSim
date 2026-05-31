/**
 * @file FileStaticWriterFactory.cpp
 * @brief Factory implementation for the generation of static file writers.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "app/io/writers/file/static/FileStaticWriterFactory.hpp"

#include <stdexcept>

#include <fmt/core.h>
#include <magic_enum/magic_enum.hpp>

#include "app/io/writers/file/static/FileStaticWriter.hpp"
#include "app/io/writers/file/static/IdrisiWriter.hpp"

namespace floodsim::app::io::writers::file {

std::unique_ptr<StaticWriter> FileStaticWriterFactory::Create(
        const formats::file::FileStaticFormat& format, const std::string& data_filename) {
    switch (format) {
    case formats::file::FileStaticFormat::kIdrisi:
        return std::make_unique<IdrisiWriter>(data_filename);
    default:
        throw std::runtime_error(fmt::format(
            "Failed to create StaticWriter: Unsupported format '{}'.",
            magic_enum::enum_name(format)));
    }
}

} // namespace floodsim::app::io::writers::file
