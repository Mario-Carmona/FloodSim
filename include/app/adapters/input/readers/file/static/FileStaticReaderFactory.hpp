
#pragma once

#include <string>
#include <filesystem>
#include <memory>

#include "app/adapters/input/readers/file/static/FileStaticFormat.hpp"
#include "app/adapters/input/readers/StaticReader.hpp"

namespace danasim {

    class FileStaticReaderFactory {
    public:
        static std::unique_ptr<StaticReader> create(
            const FileStaticFormat& format, const std::filesystem::path& dataPath,
            const std::string& dataFilename
        );
    };

} // namespace danasim
