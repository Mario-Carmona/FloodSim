/**
 * @file FileInput.cpp
 * @brief Optimized implementation of file loading logic using GDAL and C++20.
 */

#include "adapters/input/FileInput.hpp"

#include <stdexcept>
#include <fmt/core.h>
#include <algorithm>
#include <memory>

// Magic Enum Includes
#include <magic_enum/magic_enum.hpp>

#include "logging/Logger.hpp"
#include "core/grid/LayerTypes.hpp"
#include "core/common/SimulationConstants.hpp"
#include "core/grid/MapGrid.hpp"
#include "adapters/input/readers/file/static/FileStaticReader.hpp"
#include "adapters/input/readers/file/static/FileStaticReaderFactory.hpp"
#include "adapters/input/readers/file/dynamic/FileDynamicReaderFactory.hpp"

namespace danasim {

    // -------------------------------------------------------------------------
    // Implementation
    // -------------------------------------------------------------------------

    FileInput::FileInput(const std::filesystem::path& dataPath, const std::string& datasetName, const FileStaticFormat& staticFormat, const FileDynamicFormat& dynamicFormat)
        : inputPath_(dataPath / datasetName), datasetName_(datasetName), staticFormat_(staticFormat), dynamicFormat_(dynamicFormat)
    {
        LOG_INFO("FileInput initialized with root: {}", inputPath_.string());
    }

    std::unique_ptr<Reader> FileInput::generateReader(std::string name, bool isStatic) const {
        if (isStatic) {
            return std::move(FileStaticReaderFactory::create(staticFormat_, inputPath_ / name, name));
        }
        else {
            return std::move(FileDynamicReaderFactory::create(dynamicFormat_, inputPath_ / name, name));
        }
    }

    bool FileInput::isStaticLayer(const std::string& name) const {
        return FileStaticReader::isStaticLayer(inputPath_ / name, name, staticFormat_);
    }

} // namespace danasim
