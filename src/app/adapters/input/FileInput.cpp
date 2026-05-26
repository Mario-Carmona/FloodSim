/**
 * @file FileInput.cpp
 * @brief Implementation of file-system data ingestion adapter.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "app/adapters/input/FileInput.hpp"

#include <memory>
#include <string>

#include "app/io/readers/file/dynamic/FileDynamicReaderFactory.hpp"
#include "app/io/readers/file/static/FileStaticReader.hpp"
#include "app/io/readers/file/static/FileStaticReaderFactory.hpp"
#include "logging/Logger.hpp"

namespace floodsim {

// -------------------------------------------------------------------------
// Implementation
// -------------------------------------------------------------------------

FileInput::FileInput(const std::filesystem::path& data_path,
                     const std::string& dataset_name,
                     const FileStaticFormat& static_format,
                     const FileDynamicFormat& dynamic_format)
        : input_path_(data_path / dataset_name)
        , dataset_name_(dataset_name)
        , static_format_(static_format)
        , dynamic_format_(dynamic_format) {
    LOG_INFO("FileInput initialized with root: {}", input_path_.string());
}

std::unique_ptr<Reader> FileInput::GenerateReader(const std::string& name,
                                                  bool is_static) const {
    if (is_static) {
        return FileStaticReaderFactory::Create(static_format_, input_path_ / name,
            name);
    }
    return FileDynamicReaderFactory::Create(dynamic_format_, input_path_ / name,
        name);
}

bool FileInput::IsStaticLayer(const std::string& name) const {
    return FileStaticReader::IsStaticLayer(input_path_ / name, name,
        static_format_);
}

} // namespace floodsim
