
#pragma once

#include <string>
#include <filesystem>
#include <memory>

#include "app/adapters/input/readers/file/dynamic/FileDynamicFormat.hpp"
#include "app/adapters/input/readers/DynamicReader.hpp"

namespace danasim {

    class FileDynamicReaderFactory {
    public:
        static std::unique_ptr<DynamicReader> create(
            const FileDynamicFormat& format, const std::filesystem::path& dataPath,
            const std::string& dataFilename
        );
    };

} // namespace danasim
